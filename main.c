#include "gitfs.h"
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <repo-path> <mountpoint> [FUSE-opts]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *repo_path = argv[1];

    struct gitfs_state *st = calloc(1, sizeof(*st));
    if (!st)
    {
        perror("calloc");
        return EXIT_FAILURE;
    }

    if (gitfs_init_repo(st, repo_path, "HEAD") != 0)
    {
        fprintf(stderr, "Failed to open Git repo at '%s'\n", repo_path);
        free(st);
        return EXIT_FAILURE;
    }

    return fuse_main(argc - 1, argv + 1, &gitfs_oper, st);
}
