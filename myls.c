#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <termios.h>

#define VERSION         "1.0"
#define AUTHOR          "mxb360"
#define UPDATE_DATE     "2019-9"
#define PACKAGE         "Linux-MyCmd"

/* 文件排序方式 */
enum { SORT_BY_NONE, SORT_BY_NAME, SORT_BY_SIZE, SORT_BY_TIME, SORT_BY_EXTENSION };

/* 全局，用于保存命令行参数的配置信息 */
struct myls_cmdline_cfg_t {
    bool use_color;
    bool list_all;
    bool list_almost_all;
    bool list_long;
    bool list_by_line;
    bool print_directory_self;
    bool print_inode;
    bool print_blocks_size;
    bool print_classify;
    bool print_exec_type;
    bool print_dereference;
    bool sort_reverse;
    bool human_readable;
    int  powers;
    int  sort_by;

    char *name;
    char **files;
    int  files_count;
} myls_cmdline_cfg;

/* 保存文件信息每个字段的文本 */
struct word_t {
    char word[1024];
    char *_word;
    char name[256];
    char time[56];
    char size[56];
    char user[256];
    char link[12];
    char perm[12];
    char inode[12];
    char block[12];
};

/* 保存文件信息每个字段的文本长度 */
struct word_len_t {
    int  word;
    int  name;
    int  time;
    int  size;
    int  user;
    int  link;
    int  perm;
    int  inode;
    int  block;
};

/* 全局，用于保存每个文件的名字和属性信息 */
struct files_t {
    struct word_t word;
    struct word_len_t len;
    const char *color;
    time_t time;
    long   file_size;
};

/* 全局，用于保存当前扫描的目录中所有的文件信息 */
struct myls_t {
    char cwd[1024];
    char **lines;
    int lines_count;
    struct files_t *files;
    struct word_len_t max_len;
    int files_count;
    int max_files_count;
    int window_cols;
    int error;
} myls;

/* 特殊信息输出 */
#define mymsg(name, color, title, fmt, args...)  \
    fprintf(stderr, "%s: \033[%dm%s: \033[0m" fmt "\n", name, color, title, ##args)

/* 致命错误信息输出并退出 */
#define myfatal(fmt, args...)  ({ \
    mymsg(myls_cmdline_cfg.name, 31, "fatal", fmt, ##args); \
    exit(2); \
})

/* 错误信息输出 */
#define myerror(fmt, args...)  mymsg(myls_cmdline_cfg.name, 31, "error", fmt, ##args)

/* 打印使用帮助，由命令行“--help”触发 */
void usage(const char *name)
{
    printf("用法：%s [选项]... [文件]...\n", name);
    puts("列出“文件”的信息（默认当前目录）\n");

    puts("选项：\n");
    puts("  -a, --all             不隐藏任何以. 开始的项目");
    puts("  -A, --almost-all      列出除. 及.. 以外的任何项目");
    puts("  -C                    多列输出，这是默认的，除非有-l或者-1");
    puts("      --color=WHEN      带颜色的输出，WHEN可以取值auto，always，yes，never，no，默认auto");
    puts("                          auto, always, yes没有区别，表示使用颜色；");
    puts("                          no, never没有区别，表示不使用颜色");
    puts("                          可执行文件：绿色  目录：蓝色");
    puts("  -d, --directory       列出目录自身信息，而不是目录下的内容");
    puts("  -f                    不排序，启用-aU，禁用-ls --color");
    puts("  -F, --classify        在每个输出项后追加文件的类型标识符");
    puts("                          包括：可执行*  目录/  符号链接@  命令管道|  套接字=");
    puts("      --file-type       同上，但是不输出* ");
    puts("  -h, --human-readable  当有-l或者-s时，按住易读的方式打印大小，如：1K 234M 2G");
    puts("      --si              同上，但是按1000换算单位，而不是1024");
    puts("  -i, --inode           打印文件节点号");
    puts("  -l                    使用较长的格式列出更多的信息");
    puts("  -L, --dereference     对于符号链接，显示链接指向的文件信息，而不是链接文件本身");
    puts("  -s, --size            以块数形式显示每个文件分配的尺寸");
    puts("  -S                    按照文件名对输出排序");
    puts("      --sort=WORD       设置输出的排序方式。WORD可取none(-U) size(-S) time(-T)");
    puts("                          默认的排序方式是按文件名排序（不区分大小写）");
    puts("  -r, --reverse         按照指定的排序方式逆序输出");
    puts("  -T                    按照文件创建时间对输出排序");
    puts("  -U                    不对输出进行排序");
    puts("  -1                    按照每个文件占一行的格式输出");
    puts("      --help            显示此帮助信息并退出");
    puts("      --version         显示版本信息并退出\n");

    puts("退出状态：");
    puts("    0  正常");
    puts("    1  一般问题（列如：无法访问子文件夹）");
    puts("    2  严重问题（列如：无法使用命令行参数）\n");

    puts("此软件是" PACKAGE "包的程序之一，" PACKAGE "包是GNU CoreUtils部分程序的简陋的仿写版本。");
    puts("此软件是GNU CoreUtils程序 ls 的简单版本。");
    puts("此软件在输出颜色、排版和内容上与 ls 有差异。命令行参数仅支持部分 ls 命令。");
    puts("作者：" AUTHOR "  " UPDATE_DATE);
    puts("版本：" VERSION);
}

/* 打印软件版本信息 */
void version(void)
{
    printf("myls (" PACKAGE ") version " VERSION "\n");
    printf("Written by " AUTHOR "  " UPDATE_DATE " \n");
}

/* 解析命令行参数 */
void parse_cmdline(int argc, char *argv[])
{
    int index, lopt;

    myls_cmdline_cfg.use_color = true;
    myls_cmdline_cfg.sort_by = SORT_BY_NAME;
    myls_cmdline_cfg.name = argv[0];

    const char *short_options = "aACdfFhilLrsStUX1";
    const struct option long_options[] = {
        {"all", no_argument, NULL, 'a'},
        {"almost-all", no_argument, NULL, 'A'},
        {"directory", no_argument, NULL, 'd'},
        {"reverse", no_argument, NULL, 'r'},
        {"inode", no_argument, NULL, 'i'},
        {"size", no_argument, NULL, 's'},
        {"classify", no_argument, NULL, 'F'},
        {"human-readable", no_argument, NULL, 'h'},
        {"dereference", no_argument, NULL, 'L'},
        {"help", no_argument, &lopt, 0},
        {"version", no_argument, &lopt, 1},
        {"color", optional_argument, &lopt, 2},
        {"sort", required_argument, &lopt, 3},
        {"file-type", no_argument, &lopt, 4},
        {"si", no_argument, &lopt, 5},
        {NULL, 0, NULL, 0},
    };

    int c = 0;
    while (c >= 0) {
        c = getopt_long(argc, argv, short_options, long_options, &index);
        switch (c) {
        /* 处理短选项 */
        case 'a': 
            myls_cmdline_cfg.list_all = true;
            break;
        case 'A': 
            myls_cmdline_cfg.list_almost_all = true;
            break;
        case 'C':
            myls_cmdline_cfg.list_by_line = false;
            myls_cmdline_cfg.list_long = false;
            break;
        case 'd':
            myls_cmdline_cfg.print_directory_self = true;
            break;
        case 'F': 
            myls_cmdline_cfg.print_classify = true;
            myls_cmdline_cfg.print_exec_type = true;
            break;
        case 'f': 
            myls_cmdline_cfg.sort_by = SORT_BY_NONE;
            myls_cmdline_cfg.list_all = true;
            myls_cmdline_cfg.use_color = false;
            myls_cmdline_cfg.list_long = false;
            myls_cmdline_cfg.print_blocks_size = false;
        case 'h':
            myls_cmdline_cfg.human_readable = true;
            myls_cmdline_cfg.powers = 1024;
            break;
        case 'i':
            myls_cmdline_cfg.print_inode = true;
            break;
        case 'l':
            myls_cmdline_cfg.list_long = true;
            break;
        case 'L':
            myls_cmdline_cfg.print_dereference = true;
            break;
        case 'r':
            myls_cmdline_cfg.sort_reverse = true;
            break;
        case 'U':
            myls_cmdline_cfg.sort_by = SORT_BY_NONE;
            break;
        case 's':
            myls_cmdline_cfg.print_blocks_size = true;
            break;
        case 'S':
            myls_cmdline_cfg.sort_by = SORT_BY_SIZE;
            break;
        case 't':
            myls_cmdline_cfg.sort_by = SORT_BY_TIME;
            break;
        case 'X':
            myls_cmdline_cfg.sort_by = SORT_BY_EXTENSION;
            break;
        case '1':
            myls_cmdline_cfg.list_by_line = true;
            break;
        /* 错误的选项，错误信息会自动输出，这里仅输出帮助信息并退出 */
        case '?':
            myerror("命令行参数解析失败，输入\"%s --help\"查看使用帮助", argv[0]);
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
            case 2:         /* --color= */
                if (optarg == NULL)
                    myls_cmdline_cfg.use_color = true;
                else if (!strcmp(optarg, "auto") || !strcmp(optarg, "always") || !strcmp(optarg, "yes"))
                    myls_cmdline_cfg.use_color = true;
                else if (!strcmp(optarg, "no") || !strcmp(optarg, "never") || !strcmp(optarg, "no"))
                    myls_cmdline_cfg.use_color = false;
                else {
                    myerror("选项\"--%s\"的参数\"%s\"无效，输入\"%s --help\"查看使用帮助", 
                        long_options[index].name, optarg, argv[0]);
                    exit(2);
                }
                break;
            case 3:         /* --sort= */
                if (!strcmp(optarg, "none"))
                    myls_cmdline_cfg.sort_by = SORT_BY_NONE;
                else if (!strcmp(optarg, "size"))
                    myls_cmdline_cfg.sort_by = SORT_BY_SIZE;
                else if (!strcmp(optarg, "time"))
                    myls_cmdline_cfg.sort_by = SORT_BY_TIME;
                else if (!strcmp(optarg, "extension"))
                    myls_cmdline_cfg.sort_by = SORT_BY_EXTENSION;
                else {
                    myerror("选项\"--%s\"的参数\"%s\"无效，输入\"%s --help\"查看使用帮助", 
                        long_options[index].name, optarg, argv[0]);
                    exit(2);
                }
            case 4:         /* --file-type */
                myls_cmdline_cfg.print_exec_type = false;
                myls_cmdline_cfg.print_classify = true;
                break;
            case 5:
                myls_cmdline_cfg.human_readable = true;
                myls_cmdline_cfg.powers = 1000;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

    /* 由于getopt_long将argv重排，optind之后的都不再是选项了
     * 所以，这里将这些参数识别为待扫描的目录/文件
     * 并由此计算出这些参数的数量
     */
    myls_cmdline_cfg.files = argv + optind;
    myls_cmdline_cfg.files_count = argc - optind;
}

/* 辅助函数：将字符串转换为小写 */
char *_strlwr(char *str)
{
    char *s = str;
    if (s == NULL)
        return NULL;

    while (*str) {
        *str = toupper(*str);
        str++;
    }

    return s;
}

/* 辅助函数：反转任意数组
 * base：数组基地址
 * number：数组长度
 * size：数组单个元素的大小
 */
void reverse_array(void *base, int number, int size)
{
    if (base == NULL || number <= 1 || size <= 0)
        return;

    void *temp = malloc(size);
    if (temp == NULL)
        return;

    char *p = base, *q = base + (number - 1) * size;
    while (p < q) {
        memcpy(temp, q, size);
        memcpy(q, p, size);
        memcpy(p, temp, size);
        p += size;
        q -= size;
    }

    free(temp);
}

/* 排序比较函数：按照文件名排序，不区分文件名大小写 */
int sort_by_name_campare(const void *a, const void *b)
{
    char buf1[1024], buf2[1024];
    int res;

    /* 备份文件名 */
    strcpy(buf1, ((struct files_t *)a)->word.name);
    strcpy(buf2, ((struct files_t *)b)->word.name);

    /* 将文件名转化为小写，再比较棋大小（实现不区分大小写的字符串比较） */
    res = strcmp(_strlwr(buf1), _strlwr(buf2));

    /* 如果命令行参数指定了反转排序，则反转这里的比较结果 */
    return myls_cmdline_cfg.sort_reverse ? -res : res;
}

/* 排序比较函数：按照文件大小排序 */
int sort_by_size_campare(const void *a, const void *b)
{
    /* 根据文件大小比较结果 */
    int res = ((struct files_t *)a)->file_size - ((struct files_t *)b)->file_size;

    /* 如果文件大小一样，则按照文件名排序 */
    if (res == 0)
        return sort_by_name_campare(a, b);

    /* 如果命令行参数指定了反转排序，则反转这里的比较结果 */
    return myls_cmdline_cfg.sort_reverse ? res : -res;
}

/* 排序比较函数：按照文件修改时间排序 */
int sort_by_time_campare(const void *a, const void *b)
{
    /* 根据文件创建时间比较结果 */
    int res = ((struct files_t *)a)->time - ((struct files_t *)b)->time;

    /* 如果文件创建时间一样，则按照文件名排序 */
    if (res == 0)
        return sort_by_name_campare(a, b);

    /* 如果命令行参数指定了反转排序，则反转这里的比较结果 */
    return myls_cmdline_cfg.sort_reverse ? res : -res;
}

/* 排序比较函数：按照文件后缀名排序 */
int sort_by_extension_campare(const void *a, const void *b)
{
    int res = 0;
    char buf1[1024], buf2[1024];
    char *ext1 = NULL, *ext2 = NULL;

    /* 获取两个文件的后缀名 */
    for (char *name = ((struct files_t *)a)->word.name; *name; name++)
        if (*name == '.')
            ext1 = name;
    for (char *name = ((struct files_t *)b)->word.name; *name; name++)
        if (*name == '.')
            ext2 = name;

    /* 如果没有后缀名，按照文件名排序
     * 如果一个有后缀名，一个没有，没有后缀名的文件排在前面
     */
    if (ext1 == NULL && ext2 == NULL)
        return sort_by_name_campare(a, b);
    if (ext1 == NULL && ext2)
        return myls_cmdline_cfg.sort_reverse ? 1 : -1;
    if (ext1 && ext2 == NULL)
        return myls_cmdline_cfg.sort_reverse ? -1 : 1;

    /*  比较方式和比较文件名一样，不区分大小写，如果后缀名一样，按照文件名排序*/
    strcpy(buf1, ext1);
    strcpy(buf2, ext2);
    res = strcmp(_strlwr(buf1), _strlwr(buf2));
    if (res == 0)
        return sort_by_name_campare(a, b);

    /* 如果命令行参数指定了反转排序，则反转这里的比较结果 */
    return myls_cmdline_cfg.sort_reverse ? -res : res;
}

/* 按照命令行配置，对文件列表进行排序 */
void sort_files(void)
{
    switch (myls_cmdline_cfg.sort_by) {
        case SORT_BY_NONE:
            if (myls_cmdline_cfg.sort_reverse)
                reverse_array(myls.files, myls.files_count, sizeof(struct files_t));
            break;
        case SORT_BY_NAME:
            qsort(myls.files, myls.files_count, sizeof(struct files_t), sort_by_name_campare);
            break;
        case SORT_BY_SIZE:
            qsort(myls.files, myls.files_count, sizeof(struct files_t), sort_by_size_campare);
            break;
        case SORT_BY_TIME:
            qsort(myls.files, myls.files_count, sizeof(struct files_t), sort_by_time_campare);
            break;
        case SORT_BY_EXTENSION:
            qsort(myls.files, myls.files_count, sizeof(struct files_t), sort_by_extension_campare);
            break;
        default:
            break;
    }
}

/* 按照命令行指定的格式输出列表指定位置的文件 */
void stat_to_word(struct files_t *files, struct word_len_t *max_len, struct stat *st)
{
    const char *color = "\033[0m";
    const char *classify = "";

    /* 获取文件权限 */
    char str[11] = "----------";
    
    int mode = st->st_mode;

    if (mode & S_IRUSR)
        str[1] = 'r';               /* 用户的三个属性 */
    if (mode & S_IWUSR)
        str[2] = 'w';
    if (mode & S_IXUSR)
        str[3] = 'x', color = "\033[32m", classify = "*";  /* GREEN color*/
    if (mode & S_IRGRP)
        str[4] = 'r';               /* 组的三个属性   */
    if (mode & S_IWGRP)
        str[5] = 'w';
    if (mode & S_IXGRP)
        str[6] = 'x';
    if (mode & S_IROTH)
        str[7] = 'r';               /* 其他人的三个属性 */
    if (mode & S_IWOTH)
        str[8] = 'w';
    if (mode & S_IXOTH)
        str[9] = 'x';
    if (S_ISDIR(mode))
        str[0] = 'd', color = "\033[34m", classify = "/"; /* 文件夹  BLUE color      */
    else if (S_ISCHR(mode))
        str[0] = 'c';               /* 字符设备      */
    else if (S_ISBLK(mode))
        str[0] = 'b';               /* 块设备        */
    else if (S_ISLNK(mode))        
        str[0] = 'l', color = "\033[36m", classify = "@";   /* 链接  CYAN color  */
    else if (S_ISFIFO(mode))
        classify = "|";

    files->len.perm = 0;
    if (myls_cmdline_cfg.list_long) 
        files->len.perm = sprintf(files->word.perm, "%s", str);
    if (files->len.perm > max_len->perm)
        max_len->perm = files->len.perm;

    /* 输出文件节点号 */
    files->len.inode = 0;
    if (myls_cmdline_cfg.print_inode) 
        files->len.inode = sprintf(files->word.inode, "%ld", st->st_ino);
    if (files->len.inode > max_len->inode)
        max_len->inode = files->len.inode;

    files->len.block = 0;
    if (myls_cmdline_cfg.print_blocks_size) {
        /* 转换为人类易读的大小表示 */
        if (myls_cmdline_cfg.human_readable && st->st_blocks > myls_cmdline_cfg.powers) {
            int n = 0;
            float size = st->st_blocks * st->st_blksize;
            while (size > myls_cmdline_cfg.powers && n < 5) {
                size /= myls_cmdline_cfg.powers;
                n++;
            }
            files->len.block = sprintf(files->word.block, "%.1f%c", size, " KMGT"[n]);
        } else
            files->len.block = sprintf(files->word.block, "%ld", st->st_blocks / 2);
    }
        
    if (files->len.block > max_len->block)
        max_len->block = files->len.block;
    
    /* */
    files->len.time = 0;
    files->time = st->st_ctime;
    if (myls_cmdline_cfg.list_long) {
        /* 格式化文件修改时间 */
        struct tm *ptr;
        ptr = localtime(&st->st_ctime);
        files->len.time = strftime(files->word.time, 30, "%b %d %G %H:%M", ptr);
    }
    if (files->len.time > max_len->time)
        max_len->time = files->len.time;

    files->len.size = 0;
    files->file_size = st->st_size;
    if (myls_cmdline_cfg.list_long) {
        /* 转换为人类易读的大小表示 */
        if (myls_cmdline_cfg.human_readable && st->st_size > myls_cmdline_cfg.powers) {
            int n = 0;
            float size = st->st_size;
            while (size > myls_cmdline_cfg.powers && n < 5) {
                size /= myls_cmdline_cfg.powers;
                n++;
            }
            files->len.size = sprintf(files->word.size, "%.1f%c", size, " KMGT"[n]);
        } else
            files->len.size = sprintf(files->word.size, "%ld", st->st_size);
    }
       
    if (files->len.size > max_len->size)
        max_len->size = files->len.size;

    files->len.link = 0;
    if (myls_cmdline_cfg.list_long) 
       files->len.link = sprintf(files->word.link, "%ld", st->st_nlink);
    if (files->len.link > max_len->link)
        max_len->link = files->len.link;

    files->len.user = 0;
    if (myls_cmdline_cfg.list_long) {
        /* 获取文件所有者的用户名 */
        struct passwd *pwd = getpwuid(st->st_uid);
        if (pwd == NULL)
            files->len.user = sprintf(files->word.user, "%d", st->st_uid);
        else
            files->len.user = sprintf(files->word.user, "%s", pwd->pw_name);
    }
    if (files->len.user > max_len->user)
        max_len->user = files->len.user;

    files->color = myls_cmdline_cfg.use_color ? color : "";

    if (myls_cmdline_cfg.print_classify && classify[0] != '*')
        strcat(files->word.name, classify);
    else if (myls_cmdline_cfg.print_exec_type && classify[0] == '*')
        strcat(files->word.name, classify);
}

void merge_word(struct files_t *files, struct word_len_t *max_len)
{
    int len = 0;
    char *p = files->word.word;

    if (myls_cmdline_cfg.print_inode) 
        len += sprintf(p + len, "%*s ", max_len->inode, files->word.inode);

    if (myls_cmdline_cfg.print_blocks_size)
        len += sprintf(p + len, "%*s ", max_len->block, files->word.block);

    if (myls_cmdline_cfg.list_long)
        len += sprintf(p + len, "%*s ", max_len->perm, files->word.perm);

    if (myls_cmdline_cfg.list_long)
        len += sprintf(p + len, "%*s ", max_len->link, files->word.link);

    if (myls_cmdline_cfg.list_long)
        len += sprintf(p + len, "%*s ", max_len->user, files->word.user); 

    if (myls_cmdline_cfg.list_long)
        len += sprintf(p + len, "%*s ", max_len->size, files->word.size);

    if (myls_cmdline_cfg.list_long)
        len += sprintf(p + len, "%*s ", max_len->time, files->word.time);

    if (myls_cmdline_cfg.use_color) 
        p += sprintf(p + len, "\033[1m%s", files->color);

    len += sprintf(p + len, "%-*s  ", max_len->name, files->word.name);
    
    if (myls_cmdline_cfg.use_color) 
        sprintf(p + len, "\033[0m");

    files->len.word = len;
    if (max_len->word < files->len.word)
        max_len->word = files->len.word;
}

char **merge_cols(int cols, int *n, struct files_t *files, int files_count, struct word_len_t *max_len)
{
    int rows = files_count / cols + !!(files_count % cols);
    char **lines = (char **)malloc(rows * sizeof(char *));
    if (lines == NULL)
        myfatal("malloc failed: %s", strerror(errno));
    memset(lines, 0, rows * sizeof(char *));

    for (int i = 0; i < files_count; i++) {
        int index = i % rows;

        if (!lines[index]) {
            if (!(lines[index] = (char *)malloc(1000)))
                 myfatal("malloc failed: %s", strerror(errno));
            lines[index][0] = 0;
        }

        strcat(lines[index], files[i].word.word);
    }

    *n = rows;
    return lines;
}

/* 输出当前扫描的目录的所有文件信息 */
void myls_dir(const char *dir)
{
    sort_files();
    for (int i = 0; i < myls.files_count; i++) 
        merge_word(myls.files + (i), &myls.max_len);

    int cols = (myls.window_cols - 1) / myls.max_len.word;
    if (myls_cmdline_cfg.list_long || myls_cmdline_cfg.list_by_line)
        cols = 1;

    myls.lines = merge_cols(cols, &myls.lines_count, myls.files, myls.files_count, &myls.max_len);

    //printf("total: %d\n", myls.files_count);
    //printf("cols: (%d - 1) / %d = %d\n", myls.window_cols, myls.max_len.word, cols);
    //printf("rows: %d / %d + !!(%d %% %d) = %d\n\n", myls.files_count, cols, myls.files_count, cols, myls.files_count / cols + !!(myls.lines_count % cols));
    for (int i = 0; i < myls.lines_count; i++) 
        printf("%s\n", myls.lines[i]);

    //for (int i = 0; i < myls.files_count; i++) 
    //    printf("%s\n", myls.files[i].word.word);
}

/* 将当前获得的文件添加到文件列表中。
 * 添加之前会获取此文件的属性信息
 * 由于数据的长度未知，这里采取数组自动增长的策略
 */
void add_files(const char *files)
{
    #define MALLOC_SIZE       50

    /* 还没有给数组申请空间，尝试申请一段空间 */
    if (myls.max_files_count == 0) {
        if ((myls.files = (struct files_t *)malloc(MALLOC_SIZE * sizeof(struct files_t))) == NULL)
            myfatal("malloc failed: %s", strerror(errno));
        myls.max_files_count += MALLOC_SIZE;
    }

    /* 当前数组的空间已经用完，尝试重新申请一段更长的空间 */
    if (myls.max_files_count <= myls.files_count) {
        if ((myls.files = (struct files_t *)realloc(myls.files, 
                (myls.max_files_count + MALLOC_SIZE) * sizeof(struct files_t))) == NULL)
            myfatal("malloc failed: %s", strerror(errno));
        myls.max_files_count += MALLOC_SIZE;
    }

    struct stat st;

    /* 获取文件属性 */
    if ((myls_cmdline_cfg.print_dereference ? stat : lstat)(files, &st) < 0) {
        myerror("无法获取\"%s\"的属性: %s", files, strerror(errno));
        myls.error = 1;
        return;
    }

    /* 将文件名性添加到数组中 */
    strcpy(myls.files[myls.files_count].word.name, files);
    myls.files[myls.files_count].len.name = strlen(files);
    if (myls.max_len.name < myls.files[myls.files_count].len.name)
        myls.max_len.name = myls.files[myls.files_count].len.name;

    stat_to_word(&myls.files[myls.files_count], &myls.max_len, &st);
    myls.files_count++;
}

int main(int argc, char *argv[])
{
    parse_cmdline(argc, argv);
    getcwd(myls.cwd, sizeof(myls.cwd));

    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    myls.window_cols = size.ws_col;

    char *dir = ".";
    int n = 0;

    do {
        DIR *dp;
        struct dirent *direp;
        struct stat st;

        chdir(myls.cwd);

        myls.files_count = 0;

        if (myls_cmdline_cfg.files_count > n)
            dir = myls_cmdline_cfg.files[n++];

        if (stat(dir, &st) < 0) {
            myerror("无法获取\"%s\"的属性: %s", dir, strerror(errno));
            myls.error = 1;
            continue;
        }

        if ((st.st_mode & S_IFMT) != S_IFDIR || myls_cmdline_cfg.print_directory_self) {
            if (n > 1)
                printf("\n");
            add_files(dir);
            myls_dir(dir);
            continue;
        }

        if ((dp = opendir(dir)) == NULL) {
            myerror("无法打开目录\"%s\": %s", dir, strerror(errno));
            myls.error = 1;
            continue;
        }

        if (chdir(dir) < 0) {
            myerror("无法进入到目录\"%s\": %s", dir, strerror(errno));
            myls.error = 1;
            continue;
        }

        if (myls_cmdline_cfg.files_count > 1){
            if (n > 1)
                printf("\n");
            printf("%s: \n", dir);
        }

        while ((direp = readdir(dp)) != NULL) {
            if (!strcmp(direp->d_name, ".") || !strcmp(direp->d_name, "..")) {
                if (myls_cmdline_cfg.list_all)
                    add_files(direp->d_name);
            } else if (direp->d_name[0] == '.') {
                if (myls_cmdline_cfg.list_almost_all || myls_cmdline_cfg.list_all)
                    add_files(direp->d_name);
            } else
                add_files(direp->d_name);
        }

        closedir(dp);
        myls_dir(dir);

        for (int i = 0; i < myls.lines_count; i++)
            free(myls.lines[i]);
        free(myls.lines);
    } while (myls_cmdline_cfg.files_count > n);

    free(myls.files);

    return myls.error;
}
