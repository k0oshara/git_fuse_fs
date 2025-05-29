#include "gitfs.h"
#include "uthash.h"
#include <errno.h>
#include <fuse3/fuse.h>
#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define GFS ((struct gitfs_state *)fuse_get_context()->private_data)

struct path_cache_entry
{
    char *path;
    git_oid oid;
    git_object_t type;
    UT_hash_handle hh;
};
static struct path_cache_entry *path_cache = NULL;

struct blob_cache_entry
{
    char oidhex[GIT_OID_HEXSZ + 1];
    git_blob *blob;
    UT_hash_handle hh;
};
static struct blob_cache_entry *blob_cache = NULL;

static void free_cache(void)
{
    struct path_cache_entry *p, *ptmp;
    HASH_ITER(hh, path_cache, p, ptmp)
    {
        HASH_DEL(path_cache, p);
        free(p->path);
        free(p);
    }
    struct blob_cache_entry *b, *btmp;
    HASH_ITER(hh, blob_cache, b, btmp)
    {
        HASH_DEL(blob_cache, b);
        git_blob_free(b->blob);
        free(b);
    }
}

static int resolve_oid(const char *path, git_oid *oid_out,
                       git_object_t *otype_out)
{
    struct path_cache_entry *pe;
    HASH_FIND_STR(path_cache, path, pe);
    if (pe)
    {
        fprintf(stderr, "[gitfs] path cache hit: %s → %s\n", path,
                git_object_type2string(pe->type));
        *oid_out = pe->oid;
        *otype_out = pe->type;
        return 0;
    }

    fprintf(stderr, "[gitfs] resolve_oid MISS: path=\"%s\"\n", path);
    git_tree *tree = NULL;
    int err = git_commit_tree(&tree, GFS->commit);
    if (err)
        return err;

    git_oid oid;
    git_object_t otype;

    if (strcmp(path, "/") == 0)
    {
        oid = *git_tree_id(tree);
        otype = GIT_OBJECT_TREE;
    }
    else
    {
        char *p = strdup(path + 1);
        char *seg = strtok(p, "/");
        git_tree *cur = tree;
        while (seg)
        {
            const git_tree_entry *e = git_tree_entry_byname(cur, seg);
            if (!e)
            {
                err = GIT_ENOTFOUND;
                break;
            }
            otype = git_tree_entry_type(e);
            oid = *git_tree_entry_id(e);

            char hex[GIT_OID_HEXSZ + 1];
            git_oid_tostr(hex, sizeof(hex), &oid);
            fprintf(stderr, "[gitfs]   segment=\"%s\" type=%s oid=%s\n", seg,
                    otype == GIT_OBJECT_TREE ? "tree" : "blob", hex);

            seg = strtok(NULL, "/");
            if (otype == GIT_OBJECT_TREE && seg)
            {
                git_tree *next = NULL;
                err = git_tree_lookup(&next, GFS->repo, &oid);
                git_tree_free(cur);
                if (err)
                    break;
                cur = next;
            }
            else
            {
                git_tree_free(cur);
                break;
            }
        }
        free(p);
    }
    git_tree_free(tree);
    if (err)
        return err;

    pe = malloc(sizeof(*pe));
    pe->path = strdup(path);
    pe->oid = oid;
    pe->type = otype;
    HASH_ADD_KEYPTR(hh, path_cache, pe->path, strlen(pe->path), pe);

    *oid_out = oid;
    *otype_out = otype;
    return 0;
}

static int load_tree(git_tree **out, const git_oid *oid)
{
    char hex[GIT_OID_HEXSZ + 1];
    git_oid_tostr(hex, sizeof(hex), oid);
    fprintf(stderr, "[gitfs] load_tree: oid=%s\n", hex);
    return git_tree_lookup(out, GFS->repo, oid);
}

static int load_blob(git_blob **out, const git_oid *oid)
{
    char hex[GIT_OID_HEXSZ + 1];
    git_oid_tostr(hex, sizeof(hex), oid);

    struct blob_cache_entry *be;
    HASH_FIND_STR(blob_cache, hex, be);
    if (be)
    {
        fprintf(stderr, "[gitfs] cache hit: blob %s\n", hex);
        *out = be->blob;
        return 0;
    }

    fprintf(stderr, "[gitfs] load_blob: oid=%s\n", hex);
    git_blob *blob = NULL;
    int err = git_blob_lookup(&blob, GFS->repo, oid);
    if (err)
        return err;

    be = malloc(sizeof(*be));
    strcpy(be->oidhex, hex);
    be->blob = blob;
    HASH_ADD_STR(blob_cache, oidhex, be);

    *out = blob;
    return 0;
}

int gitfs_init_repo(struct gitfs_state *st, const char *repo_path,
                    const char *rev)
{
    int err;
    if ((err = git_repository_open(&st->repo, repo_path)))
        return err;
    git_object *o = NULL;
    if ((err = git_revparse_single(&o, st->repo, rev)))
        return err;
    if ((err = git_commit_lookup(&st->commit, st->repo, git_object_id(o))))
    {
        git_object_free(o);
        return err;
    }
    git_object_free(o);
    return 0;
}

void *gitfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return GFS;
}

void gitfs_destroy(void *private_data)
{
    free_cache();
    struct gitfs_state *st = private_data;
    if (st->commit)
        git_commit_free(st->commit);
    if (st->repo)
        git_repository_free(st->repo);
    git_libgit2_shutdown();
    free(st);
}

int gitfs_getattr(const char *path, struct stat *stbuf,
                  struct fuse_file_info *fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(*stbuf));

    git_oid oid;
    git_object_t type;
    if (resolve_oid(path, &oid, &type) != 0)
        return -ENOENT;

    if (type == GIT_OBJECT_TREE)
    {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 2;
    }
    else
    {
        git_blob *b = NULL;
        if (load_blob(&b, &oid) != 0)
            return -ENOENT;
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = (off_t)git_blob_rawsize(b);
    }
    return 0;
}

int gitfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi,
                  enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    git_oid oid;
    git_object_t type;
    if (resolve_oid(path, &oid, &type) != 0 || type != GIT_OBJECT_TREE)
        return -ENOENT;

    git_tree *tree = NULL;
    if (load_tree(&tree, &oid) != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    size_t cnt = git_tree_entrycount(tree);
    for (size_t i = 0; i < cnt; i++)
    {
        const git_tree_entry *e = git_tree_entry_byindex(tree, i);
        filler(buf, git_tree_entry_name(e), NULL, 0, 0);
    }
    git_tree_free(tree);
    return 0;
}

int gitfs_open(const char *path, struct fuse_file_info *fi)
{
    (void)fi;
    git_oid oid;
    git_object_t type;
    if (resolve_oid(path, &oid, &type) != 0 || type != GIT_OBJECT_BLOB)
        return -ENOENT;
    return 0;
}

int gitfs_read(const char *path, char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
    (void)fi;
    git_oid oid;
    git_object_t type;
    if (resolve_oid(path, &oid, &type) != 0 || type != GIT_OBJECT_BLOB)
        return -ENOENT;

    git_blob *b = NULL;
    if (load_blob(&b, &oid) != 0)
        return -ENOENT;

    const void *data = git_blob_rawcontent(b);
    size_t blob_size = git_blob_rawsize(b);
    size_t to_copy = 0;
    if ((size_t)offset < blob_size)
    {
        to_copy = blob_size - offset;
        if (to_copy > size)
            to_copy = size;
        memcpy(buf, (char *)data + offset, to_copy);
    }
    return (int)to_copy;
}

struct fuse_operations gitfs_oper = {
    .init = gitfs_init,
    .destroy = gitfs_destroy,
    .getattr = gitfs_getattr,
    .readdir = gitfs_readdir,
    .open = gitfs_open,
    .read = gitfs_read,
};
