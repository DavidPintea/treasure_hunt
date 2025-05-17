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
int pipe_res_monitor[2];

void sigusr1_handler(int signo) {
    got_cmd = 1;
}

void sigusr2_handler(int signo) {
    terminate_monitor = 1;
}

void handle_list_hunts() {
    DIR *d = opendir(".");
    if (!d) {
        dprintf(pipe_res_monitor[1], "Failed to open directory: %s\n", strerror(errno));
        return;
    }
    struct dirent *entry;
    char path[256];
    dprintf(pipe_res_monitor[1], "Hunts and treasure counts:\n");
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            struct stat st;
            if (stat(path, &st) == 0) {
                int count = st.st_size / sizeof(struct {
                    char treasure_id[20]; char username[100];
                    float latitude; float longitude;
                    char clue[100]; int value;
                });
                dprintf(pipe_res_monitor[1], "Hunt: %s, Count: %d\n", entry->d_name, count);
            }
        }
    }
    closedir(d);
}

void handle_list_treasures(const char *hunt_id) {
    int fd[2];
    if (pipe(fd) == -1) { dprintf(pipe_res_monitor[1], "pipe error: %s\n", strerror(errno)); return; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execlp("./treasure_manager", "treasure_manager", "--list", hunt_id, NULL);
        perror("execlp"); _exit(1);
    }
    close(fd[1]);
    char buf[512]; ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        dprintf(pipe_res_monitor[1], "%s", buf);
    }
    close(fd[0]);
    waitpid(pid, NULL, 0);
}

void handle_view_treasure(const char *hunt_id, const char *t_id) {
    int fd[2];
    if (pipe(fd) == -1) { dprintf(pipe_res_monitor[1], "pipe error: %s\n", strerror(errno)); return; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execlp("./treasure_manager", "treasure_manager", "--view", hunt_id, t_id, NULL);
        perror("execlp"); _exit(1);
    }
    close(fd[1]);
    char buf[512]; ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        dprintf(pipe_res_monitor[1], "%s", buf);
    }
    close(fd[0]);
    waitpid(pid, NULL, 0);
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
                dprintf(pipe_res_monitor[1], "cmd file open: %s\n", strerror(errno));
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
                        char *tid  = strtok(NULL, " \t\n");
                        if (hunt && tid) handle_view_treasure(hunt, tid);
                    }
                }
            }
            fclose(f);
        }
    }
    _exit(0);
}

int main() {
    pid_t monitor_pid = 0;
    int pipe_res[2];
    if (pipe(pipe_res) == -1) { perror("pipe"); return 1; }

    while (1) {
        printf("> ");
        char input[256];
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        char *cmd = strtok(input, " \t");
        if (!cmd) continue;

        if (strcmp(cmd, "start_monitor") == 0) {
            if (monitor_pid) { printf("Monitor already running (PID %d)\n", monitor_pid); continue; }
            unlink(CMD_FILE);
            if ((monitor_pid = fork()) == 0) {
                close(pipe_res[0]);
                dup2(pipe_res[1], STDOUT_FILENO);
                pipe_res_monitor[1] = pipe_res[1];
                monitor_loop();
            }
            close(pipe_res[1]);
            printf("Monitor started (PID %d)\n", monitor_pid);

        } else if (strcmp(cmd, "stop_monitor") == 0) {
            if (!monitor_pid) { printf("No monitor running\n"); continue; }
            kill(monitor_pid, SIGUSR2);
            waitpid(monitor_pid, NULL, 0);
            monitor_pid = 0;
            printf("Monitor exited\n");

        } else if (!strcmp(cmd, "list_hunts") || !strcmp(cmd, "list_treasures") || !strcmp(cmd, "view_treasure")) {
            if (!monitor_pid) { printf("Monitor is not running\n"); continue; }
            int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            char line[256] = {0};
            if (!strcmp(cmd, "list_hunts"))          
                strcpy(line, "LIST_HUNTS\n");
            else if (!strcmp(cmd, "list_treasures")) 
                sprintf(line, "LIST_TREASURES %s\n", strtok(NULL, " \t"));
            else                                      
                sprintf(line, "VIEW_TREASURE %s %s\n", strtok(NULL, " \t"), strtok(NULL, " \t"));
            write(fd, line, strlen(line)); 
            close(fd);
            kill(monitor_pid, SIGUSR1);
            usleep(100000);
            char buf[512]; ssize_t n;
            while ((n = read(pipe_res[0], buf, sizeof(buf)-1)) > 0) {
                buf[n] = '\0'; 
                printf("%s", buf);
                if (n < sizeof(buf)-1) break;
            }

        } else if (strcmp(cmd, "calculate_score") == 0) {
            DIR *d = opendir(".");
            if (!d) { perror("opendir"); continue; }
            struct dirent *e;
            while ((e = readdir(d)) != NULL) {
                if (e->d_type == DT_DIR && e->d_name[0] != '.') {
                    printf("Scores for hunt %s:\n", e->d_name);
                    int fd[2]; 
                    if (pipe(fd) == -1) { perror("pipe"); break; }
                    pid_t pid = fork();
                    if (pid == 0) {
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        execlp("./score_calculator", "score_calculator", e->d_name, NULL);
                        perror("execlp"); _exit(1);
                    }
                    close(fd[1]);
                    char buf[512]; ssize_t n;
                    while ((n = read(fd[0], buf, sizeof(buf)-1)) > 0) {
                        buf[n] = '\0';
                        printf("%s", buf);
                    }
                    close(fd[0]);
                    waitpid(pid, NULL, 0);
                }
            }
            closedir(d);

        } else if (strcmp(cmd, "exit") == 0) {
            if (monitor_pid) { printf("Stop the monitor first\n"); continue; }
            break;

        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    return 0;
}
