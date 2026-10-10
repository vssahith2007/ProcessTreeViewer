#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PROCESSES 10000
#define MAX_NAME 256

struct Process {
    int pid;
    int ppid;
    char name[MAX_NAME];
};

/* Check whether a directory name contains only digits */
int is_numeric(const char *str) {

    if (*str == '\0')
        return 0;

    while (*str) {

        if (!isdigit((unsigned char)*str))
            return 0;

        str++;
    }

    return 1;
}

/* Find a process using its PID */
int find_process(struct Process processes[],
                 int count,
                 int pid) {

    for (int i = 0; i < count; i++) {

        if (processes[i].pid == pid)
            return i;
    }

    return -1;
}

/* Recursively print the process tree */
void print_tree(struct Process processes[],
                int count,
                int parent_pid,
                int level) {

    for (int i = 0; i < count; i++) {

        if (processes[i].ppid == parent_pid) {

            /* Print indentation */
            for (int j = 0; j < level; j++) {
                printf("    ");
            }

            printf("└── %s [%d]\n",
                   processes[i].name,
                   processes[i].pid);

            /* Recursively print children */
            print_tree(processes,
                       count,
                       processes[i].pid,
                       level + 1);
        }
    }
}

int main() {

    DIR *dir;
    struct dirent *entry;

    struct Process processes[MAX_PROCESSES];

    int process_count = 0;

    char path[512];
    char line[1024];

    /* Open /proc */
    dir = opendir("/proc");

    if (dir == NULL) {
        perror("opendir /proc");
        return 1;
    }

    /* Read all entries inside /proc */
    while ((entry = readdir(dir)) != NULL) {

        /* Ignore non-numeric directories */
        if (!is_numeric(entry->d_name))
            continue;

        /* Prevent array overflow */
        if (process_count >= MAX_PROCESSES)
            break;

        /* Create /proc/<PID>/stat path */
        snprintf(path,
                 sizeof(path),
                 "/proc/%s/stat",
                 entry->d_name);

        /* Open stat file */
        FILE *file = fopen(path, "r");

        /*
         * A process may disappear while we are
         * scanning /proc, so simply skip it.
         */
        if (file == NULL)
            continue;

        /*
         * Read the complete stat line.
         */
        if (fgets(line, sizeof(line), file) != NULL) {

            /*
             * Find the first '(' and the LAST ')'.
             *
             * Using the last ')' is important because
             * Linux process names may contain ')'.
             */
            char *open_paren = strchr(line, '(');
            char *close_paren = strrchr(line, ')');

            if (open_paren != NULL &&
                close_paren != NULL &&
                close_paren > open_paren) {

                int pid;
                int ppid;
                char state;

                /*
                 * Extract PID.
                 */
                if (sscanf(line,
                           "%d",
                           &pid) == 1) {

                    /*
                     * Extract process name.
                     */
                    int name_length =
                        close_paren - open_paren - 1;

                    if (name_length >= MAX_NAME)
                        name_length = MAX_NAME - 1;

                    strncpy(processes[process_count].name,
                            open_paren + 1,
                            name_length);

                    processes[process_count]
                        .name[name_length] = '\0';

                    /*
                     * After the closing ')' comes:
                     *
                     * state PPID
                     *
                     * Example:
                     * S 323
                     */
                    if (sscanf(close_paren + 1,
                               " %c %d",
                               &state,
                               &ppid) == 2) {

                        processes[process_count].pid = pid;
                        processes[process_count].ppid = ppid;

                        process_count++;
                    }
                }
            }
        }

        fclose(file);
    }

    closedir(dir);

    /* Find PID 1 */
    int root_index = find_process(processes,
                                  process_count,
                                  1);

    if (root_index == -1) {

        printf("Root process (PID 1) not found.\n");

        return 1;
    }

    /* Print root process */
    printf("%s [%d]\n",
           processes[root_index].name,
           processes[root_index].pid);

    /* Print all descendants recursively */
    print_tree(processes,
               process_count,
               processes[root_index].pid,
               1);

    return 0;
}
