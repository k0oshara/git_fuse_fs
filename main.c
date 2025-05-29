#include "gitfs.h"
#include <fuse3/fuse.h>
#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr,
                "Usage: %s <local-repo-path> <mountpoint> [FUSE-opts]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *repo_path = argv[1];
    const char *mountpoint = argv[2];

    struct gitfs_state *st = calloc(1, sizeof(*st));
    if (!st)
    {
        perror("calloc");
        return EXIT_FAILURE;
    }

    git_libgit2_init();

    if (gitfs_init_repo(st, repo_path, "HEAD") != 0)
    {
        fprintf(stderr, "gitfs_init_repo('%s') failed\n", repo_path);
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
