#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fuse3/fuse.h>
#include <git2.h>
#include <git2/clone.h>
#include <git2/remote.h>

#include "gitfs.h"

static int is_url(const char *s)
{
    return strstr(s, "://") != NULL || strncmp(s, "git@", 4) == 0;
}

static int cred_acquire_cb(git_cred **out, const char *url, const char *user,
                           unsigned int allowed, void *payload)
{
    (void)url;
    (void)allowed;
    (void)payload;
    return git_cred_ssh_key_from_agent(out, user);
}

static int certificate_check_cb(git_cert *cert, int valid, const char *host,
                                void *payload)
{
    (void)cert;
    (void)valid;
    (void)host;
    (void)payload;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr,
                "Usage: %s <repo-path|repo-url> <mountpoint> [FUSE-opts]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *repo_arg = argv[1];
    const char *mountpoint = argv[2];
    char tmpdir[] = "/tmp/gitfs-XXXXXX";
    char *local_repo = NULL;

    struct gitfs_state *st = calloc(1, sizeof(*st));
    if (!st)
    {
        perror("calloc");
        return EXIT_FAILURE;
    }

    git_libgit2_init();

    if (is_url(repo_arg))
    {
        local_repo = mkdtemp(tmpdir);
        if (!local_repo)
        {
            perror("mkdtemp");
            return EXIT_FAILURE;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
        git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
        git_remote_callbacks remote_cbs = GIT_REMOTE_CALLBACKS_INIT;
#pragma GCC diagnostic pop

        clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;

        remote_cbs.credentials = cred_acquire_cb;
        remote_cbs.certificate_check = certificate_check_cb;
        fetch_opts.callbacks = remote_cbs;
        fetch_opts.depth = 1;
        clone_opts.fetch_opts = fetch_opts;

        int rc = git_clone(&st->repo, repo_arg, local_repo, &clone_opts);
        if (rc != 0)
        {
            fprintf(stderr, "git_clone failed: %s\n",
                    git_error_last()->message);
            return EXIT_FAILURE;
        }
    }
    else
    {
        local_repo = (char *)repo_arg;
        int rc = git_repository_open(&st->repo, local_repo);
        if (rc != 0)
        {
            fprintf(stderr, "git_repository_open failed: %s\n",
                    git_error_last()->message);
            return EXIT_FAILURE;
        }
    }

    if (gitfs_init_repo(st, local_repo, "HEAD") != 0)
    {
        fprintf(stderr, "gitfs_init_repo('%s') failed\n", local_repo);
        return EXIT_FAILURE;
    }

    int fuse_argc = 2 + (argc - 3);
    char **fuse_argv = malloc(sizeof(char *) * (fuse_argc + 1));
    if (!fuse_argv)
    {
        perror("malloc");
        return EXIT_FAILURE;
    }

    fuse_argv[0] = argv[0];
    for (int i = 3; i < argc; i++)
        fuse_argv[i - 2] = argv[i];
    fuse_argv[fuse_argc - 1] = (char *)mountpoint;
    fuse_argv[fuse_argc] = NULL;

    int ret = fuse_main(fuse_argc, fuse_argv, &gitfs_oper, st);
    free(fuse_argv);
    return ret;
}
