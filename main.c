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


const char* input_dir = "/dev/input";
const char* input_by_path_dir = "/dev/input/by-path";

static int send_key(int fd, unsigned int keycode, int pressed);
static int find_keyboard_file(char path[static PATH_MAX+1]);
static int check_if_keycode_supported(int fd, unsigned int keycode);
static int find_keycode_supported_device(char path[static PATH_MAX+1], unsigned int keycode);

int main(int argc, char* argv[])
{
    if(argc != 2 && argc != 3)
    {
        printf("  Usage: %s <keycode> [event file]\n", argv[0]);
        printf("Example: %s 125\n", argv[0]);
        printf("         %s 125 /dev/input/event0\n", argv[0]);
        return EXIT_FAILURE;
    }

    int keycode = atoi(argv[1]);

    char keyboard_file[PATH_MAX+1];
    if(argc == 2 
        && find_keycode_supported_device(keyboard_file, keycode) == EXIT_FAILURE
        && find_keyboard_file(keyboard_file) == EXIT_FAILURE)
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
        fprintf(stderr, "Failed to send key event\n");
        return EXIT_FAILURE;
    }

    event.type = EV_SYN;
    event.code = SYN_REPORT;
    gettimeofday(&(event.time), 0);
    if(write(fd, &event, sizeof(event)) < 0)
    {
        fprintf(stderr, "Failed to send sync event\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// 查找键盘文件的路径
// path - 返回文件路径，长度应当为 PATH_MAX+1
static int find_keyboard_file(char path[static PATH_MAX+1])
{
    DIR* dir = opendir(input_by_path_dir);
    if(dir == NULL)
    {
        fprintf(stderr, "Cannot open directory '%s': %s\n", input_by_path_dir, strerror(errno));
        return EXIT_FAILURE;
    }
    struct dirent* node = NULL;
    while((node = readdir(dir)) != NULL)
    {
        if(strstr(node->d_name, "event-kbd") != NULL)
        {
            snprintf(path, PATH_MAX, "%s/%s", input_by_path_dir, node->d_name);
            closedir(dir);
            return EXIT_SUCCESS;
        }
    }
    closedir(dir);
    return EXIT_FAILURE;
}


// 辅助判断设备是否支持按键码的宏
#define BITS_PER_LONG           (sizeof(long) * 8)
#define NBITS(x)                ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)                  ((x)%BITS_PER_LONG)
#define BIT(x)                  (1UL<<OFF(x))
#define LONG(x)                 ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

// 判断设备是否支持该按键码
// fd - event文件的文件描述符
// keycode - 按键码
static int check_if_keycode_supported(int fd, unsigned int keycode)
{
    unsigned long bit[NBITS(KEY_MAX)];
    if(ioctl(fd, EVIOCGBIT(EV_KEY, KEY_MAX), bit) < 0)
    {
        fprintf(stderr, "Cannot get supported events: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    
    if (test_bit(keycode, bit)) 
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// 查找支持指定按键码的设备
// path - 返回文件路径，长度应当为 PATH_MAX+1
// keycode - 按键码
static int find_keycode_supported_device(char path[static PATH_MAX+1], unsigned int keycode)
{
     DIR* dir = opendir(input_dir);
    if(dir == NULL)
    {
        fprintf(stderr, "Cannot open directory '%s': %s\n", input_by_path_dir, strerror(errno));
        return EXIT_FAILURE;
    }
    struct dirent* node = NULL;
    while((node = readdir(dir)) != NULL)
    {
        if(strstr(node->d_name, "event") == NULL)
        {
            continue;
        }

        snprintf(path, PATH_MAX, "%s/%s", input_dir, node->d_name);
        int fd = open(path, O_RDONLY);
        if(fd < 0)
        {
            fprintf(stderr, "Cannot open file '%s': %s\n", path, strerror(errno));
            continue;
        }

        if(check_if_keycode_supported(fd, keycode) == EXIT_SUCCESS)
        {
            close(fd);
            closedir(dir);
            return EXIT_SUCCESS;
        }

        close(fd);
    }
    closedir(dir);
    return EXIT_FAILURE;
}