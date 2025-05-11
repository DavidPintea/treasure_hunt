#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    char treasure_id[20];
    char username[100];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_dir>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open treasures.dat");
        return 1;
    }

    struct {
        char user[100];
        int score;
    } users[100] = {{0}};
    int ucount = 0;

    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        int i;
        for (i = 0; i < ucount; ++i)
            if (strcmp(users[i].user, t.username) == 0)
                break;
        if (i == ucount) {
            strcpy(users[ucount].user, t.username);
            users[ucount].score = 0;
            ucount++;
        }
        users[i].score += t.value;
    }
    close(fd);

    for (int i = 0; i < ucount; ++i)
        printf("%s: %d\n", users[i].user, users[i].score);

    return 0;
}
