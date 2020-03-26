#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/limits.h>

#define VERSION         "1.0"
#define AUTHOR          "mxb360"
#define UPDATE_DATE     "2020-3"
#define PACKAGE         "Linux-MyCmd"

enum { INTERACTIVE_NEVER, INTERACTIVE_ONCE, INTERACTIVE_ALWAYS };

/* 全局，用于保存命令行参数的配置信息 */
struct myrm_cmdline_cfg_t {
    bool force;
    int  interactive;
    bool preserve_root;
    bool recursive;
    bool remove_empty_dir;
    bool verbose;
    bool cancel;

    char *name;
    char **files;
    int  files_count;
} myrm_cmdline_cfg;

/* 特殊信息输出 */
#define mymsg(name, color, title, fmt, args...)  \
    fprintf(stderr, "%s: \033[%dm%s: \033[0m" fmt, name, color, title, ##args)

/* 提示信息输出 */
#define myprompt(fmt, args...) mymsg(myrm_cmdline_cfg.name, 32, "提示", fmt, ##args)

/* 错误信息输出 */
#define myerror(fmt, args...)  mymsg(myrm_cmdline_cfg.name, 31, "错误", fmt "\n", ##args)

/* 打印使用帮助，由命令行“--help”触发 */
void usage(const char *name)
{
    printf("用法: %s [选项]... <文件>...\n", name);
    puts("删除(解除链接)文件。\n");

    puts("选项：");
    puts("  -f, --force             强制忽略不存在的文件和参数，永远不提示");
    puts("  -i                      每次删除前都会提示");
    puts("  -I                      在删除超过三个文件之前提示一次，比-i弱，仍然提供了对大多数错误的保护");
    puts("      -interactive[=WHEN] 根据WHEN提示: never， once (-I)，或者always (-i)");
    puts("                            如果没有指定WHEN，则是always");
    puts("      --no-preserve-root  不特殊对待根目录");
    puts("      --preserve-root     不删除根目录（默认行为）");
    puts("  -r, -R, --recursive     递归地删除目录及其内容");
    puts("  -d, --dir               删除空目录");
    puts("  -v, --verbose           显示执行信息");
    puts("      --help              显示帮助信息后退出");
    puts("      --version           显示版本信息后退出\n");

    puts("要删除名称以“-”开头的文件，例如“-foo”，使用以下命令之一:");
    printf("    %s -- -foo\n", name);
    printf("    %s ./-foo\n", name);

    printf("\n");
    printf("此软件是" PACKAGE "包的程序之一，" PACKAGE "包是GNU CoreUtils部分程序的简陋的仿写版本。\n");
    printf("此软件是GNU CoreUtils程序 rm 的简单版本。\n");
    printf("此软件在输出颜色、排版和内容上与 rm 有差异。命令行参数仅支持部分 rm 命令。\n");
    printf("作者：" AUTHOR "  " UPDATE_DATE " \n");
    printf("版本：" VERSION "\n");
}

/* 打印软件版本信息 */
void version(void)
{
    printf("myrm (" PACKAGE ") version " VERSION "\n");
    printf("Written by " AUTHOR "  " UPDATE_DATE " \n");
}

/* 解析命令行参数 */
void parse_cmdline(int argc, char *argv[])
{
    int index, lopt;

    myrm_cmdline_cfg.interactive = INTERACTIVE_NEVER;
    myrm_cmdline_cfg.preserve_root = true;
    myrm_cmdline_cfg.name = argv[0];

    const char *short_options = "dfiIrRv";
    const struct option long_options[] = {
        {"force", no_argument, NULL, 'f'},
        {"recursive", no_argument, NULL, 'r'},
        {"dir", no_argument, NULL, 'd'},
        {"verbose", no_argument, NULL, 'h'},
        {"help", no_argument, &lopt, 0},
        {"version", no_argument, &lopt, 1},
        {"interactive", optional_argument, &lopt, 2},
        {"preserve-root", no_argument, &lopt, 3},
        {"no-preserve-root", no_argument, &lopt, 4},
        {NULL, 0, NULL, 0},
    };

    int c = 0;
    while (c >= 0) {
        c = getopt_long(argc, argv, short_options, long_options, &index);
        switch (c) {
        /* 处理短选项 */
        case 'f': 
            myrm_cmdline_cfg.force = true;
            break;
        case 'd': 
            myrm_cmdline_cfg.remove_empty_dir = true;
            break;
        case 'i':
            myrm_cmdline_cfg.interactive = INTERACTIVE_ALWAYS;
            break;
        case 'I':
            myrm_cmdline_cfg.interactive = INTERACTIVE_ONCE;
            break;
        case 'r': case 'R': 
            myrm_cmdline_cfg.recursive = true;
            break;
        case 'v': 
            myrm_cmdline_cfg.verbose = true;
            break;
        /* 错误的选项，错误信息会自动输出，这里仅输出帮助信息并退出 */
        case '?':
            myerror("命令行参数错误。输入\"%s --help\"查看使用帮助", argv[0]);
            exit(2);
            break;
        /* 下面是长选项 */
        case 0:
            switch (lopt) {
            case 0:         /* --help */
                usage(argv[0]);
                exit(0);
            case 1:         /* --version */
                version();
                exit(0);
            case 2:         /* --interactive= */
                if (optarg == NULL)
                    myrm_cmdline_cfg.interactive = INTERACTIVE_ALWAYS;
                else if (!strcmp(optarg, "once"))
                    myrm_cmdline_cfg.interactive = INTERACTIVE_ONCE;
                else if (!strcmp(optarg, "always"))
                    myrm_cmdline_cfg.interactive = INTERACTIVE_ALWAYS;
                else if (!strcmp(optarg, "never"))
                    myrm_cmdline_cfg.interactive = INTERACTIVE_NEVER;
                else {
                    myerror("命令行参数错误：选项\"--%s\"的参数\"%s\"无效。输入\"%s --help\"查看使用帮助", 
                        long_options[index].name, optarg, argv[0]);
                    exit(2);
                }
                break;
            case 3:   /* --preserve-root */
                myrm_cmdline_cfg.preserve_root = true;
                break;
            case 4:   /* --no-preserve-root */
                myrm_cmdline_cfg.preserve_root = false;
                break;
            default:
                break;
            }
        default:
            break;
        }
    }

    /* 由于getopt_long将argv重排，optind之后的都不再是选项了
     * 所以，这里将这些参数识别为待扫描的目录/文件
     * 并由此计算出这些参数的数量
     */
    myrm_cmdline_cfg.files = argv + optind;
    myrm_cmdline_cfg.files_count = argc - optind;

    if (myrm_cmdline_cfg.files_count <= 0) {
        myerror("命令行参数错误：缺少“文件”参数。输入\"%s --help\"查看使用帮助", argv[0]);
        exit(1);
    }
}

bool prompt(int num, const char *do_what, const char *file_type, const char *name)
{
    #define MAX_BUF_SIZE 128

    char buf[MAX_BUF_SIZE + 1];

    if (num > 0)
        myprompt("删除%d个对象? [yN] ", num);
    else
        myprompt("%s%s\"%s\"? [yN] ", do_what, file_type, name);
    fflush(stdout);
    fgets(buf, MAX_BUF_SIZE, stdin);

    return buf[0] == 'Y' || buf[0] == 'y';
}

char *path_join(char *path1, const char *path2)
{
    int len = strlen(path1);
    while (len > 0 && path1[len - 1] == '/')  len--;
    path1[len] = '/';
    strcpy(path1 + len + 1, path2);
    return path1;
}

bool is_root_dir(const char *path)
{
    int len = strlen(path);
    while (len > 0 && path[len - 1] == '/')  len--;
    return len == 0;
}

bool is_dot_or_double_dot(const char *path)
{
    int len = strlen(path);
    while (len > 0 && path[len - 1] == '/')  len--;

    if (len == 2 && path[0] == path[1] && path[0] == '.')
        return true;
    if (len == 1 && path[0] == '.')
        return true;
    if (path[len - 3] == '/' && path[len - 2] == path[len - 1] && path[len - 1] == '.')
        return true;
    return false;
}

int myrm_remove(const char *rm_type, int (*rm_func)(const char *), const char *name)
{
    if(myrm_cmdline_cfg.interactive && !prompt(0, "删除", rm_type, name))
        return 0;

    if (rm_func(name) < 0) {
        myerror("无法删除%s \"%s\": %s", rm_type, name, strerror(errno));
        return 1;
    }

    if (myrm_cmdline_cfg.verbose)
        printf("已删除%s \"%s\"\n", rm_type, name);
    return 0;
}

int remove_file(char *file)
{
    char current_path[PATH_MAX];
    struct stat st;

    if (stat(file, &st) < 0) {
        if (!myrm_cmdline_cfg.interactive && myrm_cmdline_cfg.force)
            return 0;
        myerror("无法删除 \"%s\": %s", file, strerror(errno));
        return 1;
    }

    /* 这是一个目录 */
    if (S_ISDIR(st.st_mode)) {
        /* 没有指定-r或者-d是不能删除目录的 */
        if (!myrm_cmdline_cfg.recursive && !myrm_cmdline_cfg.remove_empty_dir) {
            myerror("无法删除 \"%s\": 这是一个目录", file);
            return 1;
        }

        DIR *dp;
        struct dirent *direp;
        if ((dp = opendir(file)) == NULL) {
            myerror("无法访问目录 \"%s\": %s", file, strerror(errno));
            return 1;
        }

        bool is_empty = true;
        while ((direp = readdir(dp)) != NULL) {  
            /* 跳过“.”和“..” */
            if ((strcmp(".", direp->d_name)) && (strcmp("..", direp->d_name))) {
                is_empty = false;
                if (!myrm_cmdline_cfg.recursive)
                    break;

                if (is_dot_or_double_dot(file)) {
                    myerror("拒绝删除\".\"或者\"..\"目录，跳过目录 \"%s\"", file);
                    return 1;
                }

                if (myrm_cmdline_cfg.preserve_root && is_root_dir(file)) {
                    myerror("根目录保护：递归删除根目录是非常危险的操作，已阻止。可使用“--no-preserve-root”取消此保护。");
                    return 1;
                }

                if (myrm_cmdline_cfg.interactive && !prompt(0, "进入", "目录", file)) {
                    myrm_cmdline_cfg.cancel = true;
                    return 0;
                }

                /* 递归处理子目录 */
                strcpy(current_path, file);
                remove_file(path_join(file, direp->d_name));
                strcpy(file, current_path);
            }
        }  
        closedir(dp);

        /* 如果指定-d便尝试删除空目录 */
        if (!is_empty && myrm_cmdline_cfg.remove_empty_dir) {
            myerror("无法删目录 \"%s\": 这不是一个空目录", file);
            return 1;
        } else if (is_empty && myrm_cmdline_cfg.remove_empty_dir)
            return myrm_remove("空目录", rmdir, file);

        /* 删除当前目录 */
        return myrm_cmdline_cfg.cancel ? 0 : myrm_remove(is_empty ? "空目录" : "目录", rmdir, file);
    } 

    /* 删除当前文件 */
    return myrm_remove("文件", unlink, file);
}

int myrm(void)
{
    char file[PATH_MAX + 1] = {0};
    int e, error = 0;

    /* 超过三个文件提醒一次 */
    if (myrm_cmdline_cfg.interactive == INTERACTIVE_ONCE && myrm_cmdline_cfg.files_count > 3) {
        if (!prompt(myrm_cmdline_cfg.files_count, NULL, NULL, NULL)) 
            return 0;
        myrm_cmdline_cfg.interactive = INTERACTIVE_NEVER;
    } 

    for (int i = 0; i < myrm_cmdline_cfg.files_count; i++) {
        myrm_cmdline_cfg.cancel = false;
        if ((e = remove_file(strncpy(file, myrm_cmdline_cfg.files[i], PATH_MAX))) != 0)
            error = e;
    }

    return error;
}

int main(int argc, char *argv[])
{
    parse_cmdline(argc, argv);
    return myrm();
}
