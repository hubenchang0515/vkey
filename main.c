#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <linux/input.h>


const char* input_dir_path = "/dev/input/by-path";

static int send_key(int fd, unsigned int keycode, int pressed);
static int find_keyboard_file(char path[static PATH_MAX+1]);

int main(int argc, char* argv[])
{
    if(argc != 2 && argc != 3)
    {
        printf("  Usage: %s <keycode> [event file]\n", argv[0]);
        printf("Example: %s 125\n", argv[0]);
        printf("         %s 125 /dev/input/event0\n", argv[0]);
        return EXIT_FAILURE;
    }

    char keyboard_file[PATH_MAX+1];
    if(argc == 2 && find_keyboard_file(keyboard_file) == EXIT_FAILURE)
    {
        fprintf(stderr, "Failed to find keyboard device file automatically\n");
        return EXIT_FAILURE;
    }

    if(argc == 3)
    {
        strncpy(keyboard_file, argv[2], PATH_MAX);
    }

    int fd = open(keyboard_file, O_WRONLY);
    if(fd < 0)
    {
        fprintf(stderr, "Cannot open file '%s': %s\n", keyboard_file, strerror(errno));
        return EXIT_FAILURE;
    }

    int keycode = atoi(argv[1]);
    int state1 = send_key(fd, keycode, 1);
    int state2 = send_key(fd, keycode, 0);
    close(fd);

    if(state1 == EXIT_SUCCESS && state2 == EXIT_SUCCESS)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}


// 发送一个按键事件
// fd - event文件的文件描述符
// keycode - 按键码
// pressed - 0 松开  非0 按下
static int send_key(int fd, unsigned int keycode, int pressed)
{
    struct input_event event;
    event.type = EV_KEY;
    event.value = pressed != 0;
    event.code = keycode;
    gettimeofday(&(event.time), 0);
    if(write(fd, &event, sizeof(event)) < 0)
    {
        fprintf(stderr, "Failed to send key\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// 查找键盘文件的路径
// path - 返回文件路径，长度应当为 PATH_MAX+1
static int find_keyboard_file(char path[static PATH_MAX+1])
{
    DIR* dir = opendir(input_dir_path);
    if(dir == NULL)
    {
        fprintf(stderr, "Cannot open directory '%s': %s\n", input_dir_path, strerror(errno));
        return EXIT_FAILURE;
    }
    struct dirent* node = NULL;
    while((node = readdir(dir)) != NULL)
    {
        if(strstr(node->d_name, "event-kbd"))
        {
            snprintf(path, PATH_MAX, "%s/%s", input_dir_path, node->d_name);
            closedir(dir);
            return EXIT_SUCCESS;
        }
    }
    closedir(dir);
    return EXIT_FAILURE;
}