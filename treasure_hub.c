#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define CMD_FILE "monitor_cmd"

int got_cmd = 0;
int terminate_monitor = 0;

void sigusr1_handler(int signo) {
    got_cmd = 1;
}

void sigusr2_handler(int signo) {
    terminate_monitor = 1;
}

void handle_list_hunts() {
    DIR *d = opendir(".");
    if (!d) {
        perror("opendir");
        return;
    }
    struct dirent *entry;
    char path[256];
    int count;
    printf("Hunts and treasure counts:\n");
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                count = 0;
                char buf[512]; 
                while (read(fd, buf, sizeof(buf)) > 0) count++;
                close(fd);
                printf("Hunt: %s, Count: %d\n", entry->d_name, count);
            }
        }
    }
    closedir(d);
}

void handle_list_treasures(const char *hunt_id) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execlp("./treasure_manager","treasure_manager","--list", hunt_id,(char *)NULL);                 
        perror("execlp");
        _exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
        } else if (WIFEXITED(status)) {
            printf("treasure_manager s-a terminat cu cod %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("treasure_manager a fost omorât de semnal %d\n", WTERMSIG(status));
        }
    }
}

void handle_view_treasure(const char *hunt_id, const char *t_id) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./treasure_manager --view %s %s", hunt_id, t_id);
    system(cmd);
}

void monitor_loop() {
    struct sigaction sa1 = {0}, sa2 = {0};
    sa1.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = sigusr2_handler;
    sigaction(SIGUSR2, &sa2, NULL);

    while (!terminate_monitor) {
        pause();
        if (got_cmd) {
            got_cmd = 0;
            FILE *f = fopen(CMD_FILE, "r");
            if (!f) {
                perror("fopen cmd file");
                continue;
            }
            char line[256];
            if (fgets(line, sizeof(line), f)) {
                char *tok = strtok(line, " \t\n");
                if (tok) {
                    if (strcmp(tok, "LIST_HUNTS") == 0) {
                        handle_list_hunts();
                    } else if (strcmp(tok, "LIST_TREASURES") == 0) {
                        char *hunt = strtok(NULL, " \t\n");
                        if (hunt) handle_list_treasures(hunt);
                    } else if (strcmp(tok, "VIEW_TREASURE") == 0) {
                        char *hunt = strtok(NULL, " \t\n");
                        char *tid = strtok(NULL, " \t\n");
                        if (hunt && tid) handle_view_treasure(hunt, tid);
                    }
                }
            }
            fclose(f);
        }
    }
    usleep(500000);
    exit(0);
}

int main() {
    pid_t monitor_pid = 0;
    char input[256];

    while (1) {
        printf("> "); fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        char *cmd = strtok(input, " \t");
        if (!cmd) continue;
        if (strcmp(cmd, "start_monitor") == 0) {
            if (monitor_pid > 0) {
                printf("Monitor already running (PID %d)\n", monitor_pid);
                continue;
            }
            unlink(CMD_FILE);
            monitor_pid = fork();
            if (monitor_pid == 0) {
                monitor_loop();
            } else if (monitor_pid > 0) {
                printf("Monitor started (PID %d)\n", monitor_pid);
            } else {
                perror("fork");
            }
        } else if (strcmp(cmd, "list_hunts") == 0) {
            if (monitor_pid <= 0) {
                printf("Monitor is not running\n");
                continue;
            }
            FILE *f = fopen(CMD_FILE, "w");
            fprintf(f, "LIST_HUNTS\n"); fclose(f);
            kill(monitor_pid, SIGUSR1);
            usleep(100000);
        } else if (strcmp(cmd, "list_treasures") == 0) {
            char *hunt = strtok(NULL, " \t");
            if (!hunt) {
                printf("Usage: list_treasures <hunt_id>\n");
                continue;
            }
            if (monitor_pid <= 0) {
                printf("Monitor is not running\n");
                continue;
            }
            FILE *f = fopen(CMD_FILE, "w");
            fprintf(f, "LIST_TREASURES %s\n", hunt); fclose(f);
            kill(monitor_pid, SIGUSR1);
            usleep(100000); 
        } else if (strcmp(cmd, "view_treasure") == 0) {
            char *hunt = strtok(NULL, " \t");
            char *tid = strtok(NULL, " \t");
            if (!hunt || !tid) {
                printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
                continue;
            }
            if (monitor_pid <= 0) {
                printf("Monitor is not running\n");
                continue;
            }
            FILE *f = fopen(CMD_FILE, "w");
            fprintf(f, "VIEW_TREASURE %s %s\n", hunt, tid); fclose(f);
            kill(monitor_pid, SIGUSR1);
            usleep(100000); 
        } else if (strcmp(cmd, "stop_monitor") == 0) {
            if (monitor_pid <= 0) {
                printf("Monitor is not running\n");
                continue;
            }
            kill(monitor_pid, SIGUSR2);
            int status;
            waitpid(monitor_pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("Monitor exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Monitor killed by signal %d\n", WTERMSIG(status));
            }
            monitor_pid = 0;
        } else if (strcmp(cmd, "exit") == 0) {
            if (monitor_pid > 0) {
                printf("Error: monitor still running\n");
                continue;
            }
            break;
        } else {
            printf("Unknown command\n");
        }
    }
    return 0;
}
