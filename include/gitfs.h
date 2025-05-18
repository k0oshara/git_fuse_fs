#ifndef GITFS_H
#define GITFS_H

#include <fuse3/fuse.h>
#include <git2.h>

struct gitfs_state
{
    git_repository *repo;
    git_commit *commit;
};

int gitfs_init_repo(struct gitfs_state *st, const char *repo_path, const char *rev);

extern struct fuse_operations gitfs_oper;

#endif
