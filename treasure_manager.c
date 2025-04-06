#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include<stdlib.h>

typedef struct {
    int treasure_id;
    char username[100];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void add_treasure(const char *hunt_id)
{
    char filename[50];
    snprintf(filename, sizeof(filename), "hunt_%s.dat", hunt_id);
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Errorfile");
        return;
    }

    Treasure t;
    printf("Enter treasure ID: ");
    scanf("%d",&t.treasure_id);
    printf("Enter username: ");
    scanf(" %s", t.username);
    printf("Enter latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter longitude: ");
    scanf("%f", &t.longitude);
    printf("Enter clue: ");
    scanf(" %s", t.clue);
    printf("Enter value: ");
    scanf("%d", &t.value);

    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Failed to write treasure");
    }
    close(fd);
}

void list_treasures(const char *hunt_id)
{

}

void view_treasure(const char *hunt_id, const char *treasure_id)
{

}

void remove_hunt(const char *hunt_id) 
{

}

void remove_treasure(const char *hunt_id, const char *treasure_id) 
{

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("No good command\n");
        return 1;
    }

    const char *command = argv[1];
    const char *hunt_id = (argc > 2) ? argv[2] : NULL;
    const char *treasure_id = (argc > 3) ? argv[3] : NULL;

    if (!hunt_id) {
        printf("Hunt ID needed\n");
        return 1;
    }

    if (strcmp(command, "add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "view") == 0 && treasure_id) {
        view_treasure(hunt_id, treasure_id);
    } else if (strcmp(command, "remove_treasure") == 0 && treasure_id) {
        remove_treasure(hunt_id, treasure_id);
    } else if (strcmp(command, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        printf("wrong command\n");
    }

    return 0;
}