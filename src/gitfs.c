#include "gitfs.h"
#include <fuse3/fuse.h>
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#define GFS ((struct gitfs_state *)fuse_get_context()->private_data)

int gitfs_init_repo(struct gitfs_state *st, const char *repo_path, const char *rev)
{
    git_libgit2_init();

    int error;
    if ((error = git_repository_open(&st->repo, repo_path)))
        return error;

    git_object *obj = NULL;
    if ((error = git_revparse_single(&obj, st->repo, rev)))
        return error;

    error = git_commit_lookup(&st->commit, st->repo, git_object_id(obj));
    git_object_free(obj);
    return error;
}

void *gitfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return GFS;
}

void gitfs_destroy(void *private_data)
{
    struct gitfs_state *st = private_data;
    if (st->commit)
        git_commit_free(st->commit);
    if (st->repo)
        git_repository_free(st->repo);
    git_libgit2_shutdown();
    free(st);
}

int gitfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(*stbuf));

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    git_tree *tree;
    if (git_commit_tree(&tree, GFS->commit))
        return -ENOENT;

    const git_tree_entry *entry = git_tree_entry_byname(tree, path + 1);
    if (!entry)
    {
        git_tree_free(tree);
        return -ENOENT;
    }

    if (git_tree_entry_type(entry) == GIT_OBJECT_BLOB)
    {
        git_blob *blob = NULL;
        if (git_blob_lookup(&blob, GFS->repo, git_tree_entry_id(entry)))
        {
            git_tree_free(tree);
            return -ENOENT;
        }
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = (off_t)git_blob_rawsize(blob);
        git_blob_free(blob);
    }
    else
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }

    git_tree_free(tree);
    return 0;
}

int gitfs_readdir(const char *path,
                  void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi,
                  enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    git_tree *tree;
    if (git_commit_tree(&tree, GFS->commit))
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    size_t count = git_tree_entrycount(tree);
    for (size_t i = 0; i < count; i++)
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
    git_tree *tree;
    if (git_commit_tree(&tree, GFS->commit))
        return -ENOENT;

    const git_tree_entry *entry = git_tree_entry_byname(tree, path + 1);
    git_tree_free(tree);

    if (!entry || git_tree_entry_type(entry) != GIT_OBJECT_BLOB)
        return -ENOENT;

    return 0;
}

int gitfs_read(const char *path, char *buf, size_t size,
               off_t offset, struct fuse_file_info *fi)
{
    (void)fi;
    git_tree *tree;
    if (git_commit_tree(&tree, GFS->commit))
        return -ENOENT;

    const git_tree_entry *entry = git_tree_entry_byname(tree, path + 1);
    if (!entry)
    {
        git_tree_free(tree);
        return -ENOENT;
    }

    git_blob *blob = NULL;
    if (git_blob_lookup(&blob, GFS->repo, git_tree_entry_id(entry)))
    {
        git_tree_free(tree);
        return -ENOENT;
    }
    git_tree_free(tree);

    const void *data = git_blob_rawcontent(blob);
    size_t blob_size = git_blob_rawsize(blob);
    size_t to_copy = 0;

    if ((size_t)offset < blob_size)
    {
        to_copy = blob_size - offset;
        if (to_copy > size)
            to_copy = size;
        memcpy(buf, (char *)data + offset, to_copy);
    }

    git_blob_free(blob);
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
