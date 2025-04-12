#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

typedef struct
{
    char treasure_id[20];
    char username[100];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void log_operation(const char *hunt_dir, const char *operation)
{
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_dir);
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1)
    {
        perror("open log");
        return;
    }
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "%s: %s\n", time_str, operation);
    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);
}

int hunt_exists(const char *hunt_dir)
{
    DIR *dir = opendir(hunt_dir);
    if (dir)
    {
        closedir(dir);
        return 1; 
    } else if (errno == ENOENT)
    {
        return 0; 
    } else 
    {
        perror("opendir");
        return -1; 
    }
}

void add_treasure(const char *hunt_id)
{
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);
    int exists = hunt_exists(hunt_dir);
    if (exists == -1)
    {
        return; 
    } else if (exists == 0) 
    {
        if (mkdir(hunt_dir, 0755) == -1)
        {
            perror("mkdir");
            return;
        }
        char log_path[256];
        snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_dir);
        int log_fd = open(log_path, O_WRONLY | O_CREAT, 0644);
        if (log_fd == -1)
        {
            perror("open log");
            return;
        }
        close(log_fd);
        char link_name[256];
        snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
        if (symlink(log_path, link_name) == -1) {
            perror("symlink");
        }
    }
    char treasures_path[256];
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", hunt_dir);
    int fd = open(treasures_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
     {
        perror("open treasures");
        return;
    }
    Treasure t;
    printf("Enter treasure ID: ");
    scanf(" %s",t.treasure_id);
    printf("Enter username: ");
    scanf(" %s", t.username);
    printf("Enter latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter longitude: ");
    scanf("%f", &t.longitude);
    getchar();
    printf("Enter clue: ");
    fgets(t.clue,100,stdin);
    printf("Enter value: ");
    scanf("%d", &t.value);

    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure))
    {
        perror("Failed to write treasure");
    }
    close(fd);
    char operation[256];
    snprintf(operation, sizeof(operation), "Added treasure '%s'", t.treasure_id);
    log_operation(hunt_dir, operation);
}

void list_treasures(const char *hunt_id)
{
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    int exists = hunt_exists(hunt_dir);
    if (exists == -1)
    {
        return; 
    } else if (exists == 0)
    {
        printf("Hunt '%s' doesnt't exist.\n", hunt_id);
        return;
    }
    char treasures_path[256];
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", hunt_dir);
    struct stat file_stat;
    if (stat(treasures_path, &file_stat) != 0)
    {
        perror("stat");
        return;
    }
    printf("Hunt: %s\n", hunt_id);
    printf("File dimension: %lld bytes\n", file_stat.st_size);
    printf("Last modification: %s", ctime(&file_stat.st_mtime));
    int fd = open(treasures_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return;
    }
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("ID: %s, Username: %s, Lat: %.4f, Lon: %.4f, Clue: %s, Value: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
    char operation[256];
    snprintf(operation, sizeof(operation), "Listed treasures in hunt '%s'", hunt_id);
    log_operation(hunt_dir, operation);
}

void view_treasure(const char *hunt_id, const char *id)
{
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    int exists = hunt_exists(hunt_dir);
    if (exists == -1)
    {
        return; 
    } else if (exists == 0)
    {
        printf("Hunt '%s' doesn't exist.\n", hunt_id);
        return;
    }
    char treasures_path[256];
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", hunt_dir);

    int fd = open(treasures_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return;
    }
    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (strcmp(t.treasure_id, id) == 0)
        {
            printf("ID: %s\nUsername: %s\nLat: %.4f\nLon: %.4f\nClue: %s\nValue: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    close(fd);
    if (!found) 
    {
        printf("Treasure with ID '%s' not found in hunt '%s'.\n", id, hunt_id);
    } else 
    {
        char operation[256];
        snprintf(operation, sizeof(operation), "Viewed treasure '%s' in hunt '%s'", id, hunt_id);
        log_operation(hunt_dir, operation);
    }
}


void remove_treasure(const char *hunt_id, const char *id)
{
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    int exists = hunt_exists(hunt_dir);
    if (exists == -1)
    {
        perror("hunt_exists");
        return; 
    } else if (exists == 0)
    {
        printf("Hunt '%s' doesn't exist.\n", hunt_id);
        return;
    }

    char treasures_path[256];
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", hunt_dir);

    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s/temp.dat", hunt_dir);

    int src_fd = open(treasures_path, O_RDONLY);
    if (src_fd == -1)
    {
        perror("open treasures.dat");
        printf("No treasures exist in hunt '%s'.\n", hunt_id);
        return;
    }

    int dest_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1)
    {
        perror("open temp");
        close(src_fd);
        return;
    }

    Treasure t;
    int removed = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, &t, sizeof(Treasure))) == sizeof(Treasure))
    {
        if (strcmp(t.treasure_id, id) != 0)
        {
            if (write(dest_fd, &t, sizeof(Treasure)) != sizeof(Treasure))
            {
                perror("write temp");
                close(src_fd);
                close(dest_fd);
                unlink(temp_path);
                return;
            }
        } else {
            removed = 1;
        }
    }
    if (bytes_read == -1)
    {
        perror("read");
        close(src_fd);
        close(dest_fd);
        unlink(temp_path);
        return;
    }

    close(src_fd);
    close(dest_fd);

    if (removed)
    {
        if (unlink(treasures_path) == -1)
        {
            perror("unlink treasures");
            unlink(temp_path);
            return;
        }
        if (rename(temp_path, treasures_path) == -1)
        {
            perror("rename");
            return;
        }
        char operation[256];
        snprintf(operation, sizeof(operation), "Removed treasure '%s' from hunt '%s'", id, hunt_id);
        log_operation(hunt_dir, operation);
        printf("Treasure '%s' removed from hunt '%s'.\n", id, hunt_id);
    } else 
    {
        unlink(temp_path);
        printf("Treasure with ID '%s' not found in hunt '%s'.\n", id, hunt_id);
        char operation[256];
        snprintf(operation, sizeof(operation), "Failed to remove treasure '%s' from hunt '%s' (not found)", id, hunt_id);
        log_operation(hunt_dir, operation);
    }
}  

   

void remove_hunt(const char *hunt_id)
{
    char hunt_dir[256];
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_id);

    int exists = hunt_exists(hunt_dir);
    if (exists == -1) {
        return;
    } else if (exists == 0) {
        printf("Hunt '%s' doesn't exist.\n", hunt_id);
        return;
    }

    char treasures_path[256];
    snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", hunt_dir);
    unlink(treasures_path);

    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_dir);
    unlink(log_path);

    rmdir(hunt_dir);

    char link_name[256];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    unlink(link_name);

    printf("Hunt '%s' was deleted.\n", hunt_id);
}

int main(int argc, char *argv[])
{
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

    if (strcmp(command, "--add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(command, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "--view") == 0 && treasure_id) {
        view_treasure(hunt_id, treasure_id);
    } else if (strcmp(command, "--remove_treasure") == 0 && treasure_id) {
        remove_treasure(hunt_id, treasure_id);
    } else if (strcmp(command, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        printf("wrong command\n");
    }

    return 0;
}