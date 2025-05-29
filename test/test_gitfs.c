#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#define TEST_REPO_PATH "./test-repo"
#define TEST_MOUNT_PATH "./test-mount"
#define GITFS_BINARY "./gitfs"

static int test_count = 0;
static int test_passed = 0;
static pid_t gitfs_pid = 0;

void cleanup_mount(void) {
    if (gitfs_pid > 0) {
        kill(gitfs_pid, SIGTERM);
        waitpid(gitfs_pid, NULL, 0);
        gitfs_pid = 0;
    }
    
    // Try to unmount
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "fusermount3 -u %s 2>/dev/null || fusermount -u %s 2>/dev/null || umount %s 2>/dev/null", 
             TEST_MOUNT_PATH, TEST_MOUNT_PATH, TEST_MOUNT_PATH);
    system(cmd);
    
    sleep(1);
}

void signal_handler(int sig) {
    (void)sig;
    cleanup_mount();
    exit(1);
}

int setup_test_repo(void) {
    printf("Setting up test repository...\n");
    
    // Remove existing repo
    system("rm -rf " TEST_REPO_PATH);
    
    // Create new repo
    if (mkdir(TEST_REPO_PATH, 0755) != 0) {
        perror("mkdir test-repo");
        return -1;
    }
    
    if (chdir(TEST_REPO_PATH) != 0) {
        perror("chdir test-repo");
        return -1;
    }
    
    // Initialize git repo
    if (system("git init") != 0) {
        fprintf(stderr, "Failed to initialize git repo\n");
        return -1;
    }
    
    if (system("git config user.email 'test@example.com'") != 0 ||
        system("git config user.name 'Test User'") != 0) {
        fprintf(stderr, "Failed to configure git\n");
        return -1;
    }
    
    // Create test files
    FILE *f = fopen("hello.txt", "w");
    if (!f) {
        perror("fopen hello.txt");
        return -1;
    }
    fprintf(f, "Hello, World!\n");
    fclose(f);
    
    if (mkdir("subdir", 0755) != 0) {
        perror("mkdir subdir");
        return -1;
    }
    
    f = fopen("subdir/nested.txt", "w");
    if (!f) {
        perror("fopen subdir/nested.txt");
        return -1;
    }
    fprintf(f, "This is a nested file.\nWith multiple lines.\n");
    fclose(f);
    
    f = fopen("binary.dat", "wb");
    if (!f) {
        perror("fopen binary.dat");
        return -1;
    }
    // Write some binary data
    for (int i = 0; i < 256; i++) {
        fputc(i, f);
    }
    fclose(f);
    
    // Add and commit
    if (system("git add .") != 0) {
        fprintf(stderr, "Failed to add files\n");
        return -1;
    }
    
    if (system("git commit -m 'Initial test commit'") != 0) {
        fprintf(stderr, "Failed to commit\n");
        return -1;
    }
    
    // Go back to parent directory
    if (chdir("..") != 0) {
        perror("chdir ..");
        return -1;
    }
    
    printf("Test repository created successfully.\n");
    return 0;
}

int mount_gitfs(void) {
    printf("Mounting GitFS...\n");
    
    // Create mount point
    system("rm -rf " TEST_MOUNT_PATH);
    if (mkdir(TEST_MOUNT_PATH, 0755) != 0) {
        perror("mkdir test-mount");
        return -1;
    }
    
    // Fork and exec gitfs
    gitfs_pid = fork();
    if (gitfs_pid == 0) {
        // Child process
        execl(GITFS_BINARY, GITFS_BINARY, TEST_REPO_PATH, TEST_MOUNT_PATH, "-f", NULL);
        perror("execl gitfs");
        exit(1);
    } else if (gitfs_pid < 0) {
        perror("fork");
        return -1;
    }
    
    // Wait for mount to be ready
    for (int i = 0; i < 50; i++) {
        usleep(100000); // 100ms
        struct stat st;
        if (stat(TEST_MOUNT_PATH "/hello.txt", &st) == 0) {
            printf("GitFS mounted successfully.\n");
            return 0;
        }
    }
    
    fprintf(stderr, "GitFS mount timeout\n");
    cleanup_mount();
    return -1;
}

void test_assert(const char *test_name, int condition) {
    test_count++;
    printf("Test %d: %s... ", test_count, test_name);
    if (condition) {
        printf("PASS\n");
        test_passed++;
    } else {
        printf("FAIL\n");
    }
}

void test_file_exists(void) {
    struct stat st;
    test_assert("File exists", stat(TEST_MOUNT_PATH "/hello.txt", &st) == 0);
}

void test_file_content(void) {
    FILE *f = fopen(TEST_MOUNT_PATH "/hello.txt", "r");
    if (!f) {
        test_assert("File content", 0);
        return;
    }
    
    char buf[256];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    
    test_assert("File content", strcmp(buf, "Hello, World!\n") == 0);
}

void test_directory_exists(void) {
    struct stat st;
    test_assert("Directory exists", 
                stat(TEST_MOUNT_PATH "/subdir", &st) == 0 && S_ISDIR(st.st_mode));
}

void test_nested_file(void) {
    FILE *f = fopen(TEST_MOUNT_PATH "/subdir/nested.txt", "r");
    if (!f) {
        test_assert("Nested file", 0);
        return;
    }
    
    char buf[256];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    
    test_assert("Nested file", strcmp(buf, "This is a nested file.\n") == 0);
}

void test_directory_listing(void) {
    DIR *dir = opendir(TEST_MOUNT_PATH);
    if (!dir) {
        test_assert("Directory listing", 0);
        return;
    }
    
    int found_hello = 0, found_subdir = 0, found_binary = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "hello.txt") == 0) found_hello = 1;
        if (strcmp(entry->d_name, "subdir") == 0) found_subdir = 1;
        if (strcmp(entry->d_name, "binary.dat") == 0) found_binary = 1;
    }
    
    closedir(dir);
    test_assert("Directory listing", found_hello && found_subdir && found_binary);
}

void test_binary_file(void) {
    FILE *f = fopen(TEST_MOUNT_PATH "/binary.dat", "rb");
    if (!f) {
        test_assert("Binary file", 0);
        return;
    }
    
    int correct = 1;
    for (int i = 0; i < 256; i++) {
        int c = fgetc(f);
        if (c != i) {
            correct = 0;
            break;
        }
    }
    
    fclose(f);
    test_assert("Binary file", correct);
}

void test_file_permissions(void) {
    struct stat st;
    if (stat(TEST_MOUNT_PATH "/hello.txt", &st) != 0) {
        test_assert("File permissions", 0);
        return;
    }
    
    // Should be read-only
    test_assert("File permissions", (st.st_mode & 0777) == 0444);
}

void test_write_protection(void) {
    FILE *f = fopen(TEST_MOUNT_PATH "/hello.txt", "w");
    test_assert("Write protection", f == NULL);
    if (f) fclose(f);
}

int main(void) {
    printf("GitFS Test Suite\n");
    printf("================\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Setup test environment
    if (setup_test_repo() != 0) {
        fprintf(stderr, "Failed to setup test repository\n");
        return 1;
    }
    
    if (mount_gitfs() != 0) {
        fprintf(stderr, "Failed to mount GitFS\n");
        return 1;
    }
    
    // Run tests
    printf("\nRunning tests...\n");
    test_file_exists();
    test_file_content();
    test_directory_exists();
    test_nested_file();
    test_directory_listing();
    test_binary_file();
    test_file_permissions();
    test_write_protection();
    
    // Cleanup
    cleanup_mount();
    
    // Results
    printf("\nTest Results:\n");
    printf("=============\n");
    printf("Passed: %d/%d\n", test_passed, test_count);
    
    if (test_passed == test_count) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed! ✗\n");
        return 1;
    }
}