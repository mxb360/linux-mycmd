#include "myutils.h"

#include <time.h>

#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>

#define MALLOC_SIZE       50

#define SORT_BY_NONE      0
#define SORT_BY_NAME      1
#define SORT_BY_SIZE      2
#define SORT_BY_TIME      3
#define SORT_BY_EXTENSION 4

/* 全局，用于保存命令行参数的配置信息 */
struct myls_cmdline_cfg_t {
    bool use_color;
    bool list_all;
    bool list_almost_all;
    bool list_long;
    bool print_directory_self;
    bool print_inode;
    bool print_blocks_size;
    bool sort_reverse;
    int  sort_by;

    char **files;
    int  files_count;
} myls_cmdline_cfg;

/* 全局，用于保存每个文件的名字和属性信息 */
struct files_t {
    char name[1024];
    struct stat st;
};

/* 全局，用于保存当前扫描的目录中所有的文件信息 */
struct myls_t {
    char cwd[1024];
    struct files_t *files;
    int files_count;
    int max_files_count;
    int error;
} myls;

/* 打印使用帮助，用命令行“--help”触发 */
void uasge(void)
{
    printf("用法：%s [选项]... [文件]...\n", myutils_get_name());
    printf("列出“文件”的信息（默认当前目录）\n\n");

    printf("选项：\n");
    printf("  -a, --all           不隐藏任何以. 开始的项目\n");
    printf("  -A, --almost-all    列出除. 及.. 以外的任何项目\n");
    printf("  -d, --directory     列出目录自身信息，而不是目录下的内容\n");
    printf("  -f                  不排序，启用-aU，禁用-ls --color\n");
    printf("  -i, --inode         打印文件节点号\n");
    printf("  -l                  使用较长的格式列出更多的信息\n");
    printf("  -s, --size          以块数形式显示每个文件分配的尺寸\n");
    printf("  -S                  按照文件名对输出排序\n");
    printf("  -r, --reverse       按照指定的排序方式逆序输出\n");
    printf("  -T                  按照文件创建时间对输出排序\n");
    printf("  -U                  不对输出进行排序\n");
    printf("  -1                  按照每个文件占一行的格式输出（目前仅支持此方式，所以是默认行为）\n");
    printf("      --sort=WORD     设置输出的排序方式。WORD可取none(-U) size(-S) time(-T)\n");
    printf("                      默认的排序方式是按文件名排序（不区分大小写）\n");
    printf("      --color=WHEN    带颜色的输出，WHEN可以取值yes, no；默认值是yes\n");
    printf("      --help          显示此帮助信息并退出\n");
    printf("      --version       显示版本信息并退出\n");

    printf("\n");
    printf("退出状态：\n");
    printf("    0  正常\n");
    printf("    1  一般问题（列如：无法访问子文件夹）\n");
    printf("    2  严重问题（列如：无法使用命令行参数）\n");

    printf("\n");
    printf("此软件是Linux-MyCmd包的程序之一，Linux-MyCmd包是GNU CoreUtils程序的简陋的仿写版本。\n");
    printf("此软件是GNU CoreUtils程序 ls 的简单版本。\n此软件在输出上与 ls 有差异。命令行参数也有差异。\n");
    printf("作者：mxb360   \n");
}

/* 打印软件版本信息 */
void version(void)
{
    printf("myls  version 0.0.1\n");
}

/* 默认的命令行配置：在相应命令行参数未指定时的默认值 */
void default_cfg(void)
{
    myls_cmdline_cfg.use_color = true;
    myls_cmdline_cfg.list_all = false;
    myls_cmdline_cfg.list_almost_all = false;
    myls_cmdline_cfg.list_long = false;
    myls_cmdline_cfg.print_inode = false;
    myls_cmdline_cfg.print_directory_self = false;
    myls_cmdline_cfg.print_blocks_size = false;
    myls_cmdline_cfg.sort_reverse = false;
    myls_cmdline_cfg.sort_by = SORT_BY_NAME;
}

/* 解析命令行参数 */
void parse_cmdline(int argc, char *argv[])
{
    int index, lopt;

    const char *short_options = "aAdfilrsStUX1";
    const struct option long_options[] = {
        {"all", no_argument, NULL, 'a'},
        {"almost-all", no_argument, NULL, 'A'},
        {"directory", no_argument, NULL, 'd'},
        {"reverse", no_argument, NULL, 'r'},
        {"inode", no_argument, NULL, 'i'},
        {"size", no_argument, NULL, 's'},
        {"help", no_argument, &lopt, 0},
        {"version", no_argument, &lopt, 1},
        {"color", optional_argument, &lopt, 2},
        {"sort", required_argument, &lopt, 3},
        {NULL, 0, NULL, 0},
    };

    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, &index);
        if (c == -1)
            break;
        switch (c) {
            /* 处理短选项 */
            case 'a': myls_cmdline_cfg.list_all = true;             break;
            case 'A': myls_cmdline_cfg.list_almost_all = true;      break;
            case 'd': myls_cmdline_cfg.print_directory_self = true; break;
            case 'i': myls_cmdline_cfg.print_inode = true;          break;
            case 'l': myls_cmdline_cfg.list_long = true;            break;
            case 'r': myls_cmdline_cfg.sort_reverse = true;         break;
            case 'U': myls_cmdline_cfg.sort_by = SORT_BY_NONE;      break;
            case 's': myls_cmdline_cfg.print_blocks_size = true;    break;
            case 'S': myls_cmdline_cfg.sort_by = SORT_BY_SIZE;      break;
            case 't': myls_cmdline_cfg.sort_by = SORT_BY_TIME;      break;
            case 'X': myls_cmdline_cfg.sort_by = SORT_BY_EXTENSION; break;
            case '1':                                               break;
            case 'f': 
                myls_cmdline_cfg.sort_by = SORT_BY_NONE; break;
                myls_cmdline_cfg.list_all = true;
                myls_cmdline_cfg.use_color = false;
                myls_cmdline_cfg.list_long = false;
                myls_cmdline_cfg.print_blocks_size = false;
                break;
            /* 错误的选项，错误信息会自动输出，这里仅输出帮助提升信息并退出 */
            case '?':
                myerror(0, 0, "命令行参数解析失败，输入\"%s --help\"查看使用帮助", argv[0]);
                exit(2);
                break;
            /* 下面是长选项 */
            case 0:
                switch (lopt) {
                    case 0:
                        uasge();
                        exit(0);
                    case 1:
                        version();
                        exit(0);
                    case 2:
                        if (optarg == NULL)
                            myls_cmdline_cfg.use_color = true;
                        else if (!strcmp(optarg, "yes"))
                            myls_cmdline_cfg.use_color = true;
                        else if (!strcmp(optarg, "no"))
                            myls_cmdline_cfg.use_color = false;
                        else {
                            myerror(0, 0, "选项\"--%s\"的参数参数\"%s\"无效，输入\"%s --help\"查看使用帮助", 
                                long_options[index].name, optarg, argv[0]);
                            exit(2);
                        }
                        break;
                    case 3:
                        if (!strcmp(optarg, "none"))
                            myls_cmdline_cfg.sort_by = SORT_BY_NONE;
                        else if (!strcmp(optarg, "size"))
                            myls_cmdline_cfg.sort_by = SORT_BY_SIZE;
                        else if (!strcmp(optarg, "time"))
                            myls_cmdline_cfg.sort_by = SORT_BY_TIME;
                        else if (!strcmp(optarg, "extension"))
                            myls_cmdline_cfg.sort_by = SORT_BY_EXTENSION;
                        else {
                            myerror(0, 0, "选项\"--%s\"的参数\"%s\"无效，输入\"%s --help\"查看使用帮助", 
                                long_options[index].name, optarg, argv[0]);
                            exit(2);
                        }
                }
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

/* 将当前获得的文件添加到文件列表中。
 * 添加之前会获取此文件的属性信息
 * 由于数据的长度未知，这里采取数组自动增长的策略
 */
void add_files(const char *files)
{
    /* 还没有给数组申请空间，尝试申请一段空间 */
    if (myls.max_files_count == 0) {
        if ((myls.files = (struct files_t *)malloc(MALLOC_SIZE * sizeof(struct files_t))) == NULL)
            myfatal(errno, 2, "内存申请错误");
        myls.max_files_count += MALLOC_SIZE;
    }

    /* 当前数组的空间已经用完，尝试重新申请一段更长的空间 */
    if (myls.max_files_count <= myls.files_count) {
        if ((myls.files = (struct files_t *)realloc(myls.files, (myls.max_files_count + MALLOC_SIZE) * sizeof(struct files_t))) == NULL)
            myfatal(errno, 2, "内存申请错误");
        myls.max_files_count += MALLOC_SIZE;
    }

    /* 将文件名和文件属性添加到数组中，如果属性获取失败，则输出错误信息并忽略该文件 */
    strcpy(myls.files[myls.files_count].name, files);
    if (stat(files, &myls.files[myls.files_count].st) < 0) {
        myerror(errno, 0, "无法获取\"%s\"的属性", files);
        myls.error = 1;
        return;
    }

    myls.files_count++;
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
    strcpy(buf1, ((struct files_t *)a)->name);
    strcpy(buf2, ((struct files_t *)b)->name);

    /* 将文件名转化为小写，再比较棋大小（实现不区分大小写的字符串比较） */
    res = strcmp(_strlwr(buf1), _strlwr(buf2));

    /* 如果命令行参数指定了反转排序，则反转这里的比较结果 */
    return myls_cmdline_cfg.sort_reverse ? -res : res;
}

/* 排序比较函数：按照文件大小排序 */
int sort_by_size_campare(const void *a, const void *b)
{
    /* 根据文件大小比较结果 */
    int res = ((struct files_t *)a)->st.st_size - ((struct files_t *)b)->st.st_size;

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
    int res = ((struct files_t *)a)->st.st_ctime - ((struct files_t *)b)->st.st_ctime;

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
    for (char *name = ((struct files_t *)a)->name; *name; name++)
        if (*name == '.')
            ext1 = name;
    for (char *name = ((struct files_t *)b)->name; *name; name++)
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
void myls_files(const char *name, struct stat *st)
{
    color_t color = DEF_COLOR;

    /* 获取文件权限 */
    char str[11] = "----------";
    int mode = st->st_mode;

    if (mode & S_IRUSR)     str[1] = 'r';               /* 用户的三个属性 */
    if (mode & S_IWUSR)     str[2] = 'w';
    if (mode & S_IXUSR)     str[3] = 'x', color = GREEN;
    if (mode & S_IRGRP)     str[4] = 'r';               /* 组的三个属性   */
    if (mode & S_IWGRP)     str[5] = 'w';
    if (mode & S_IXGRP)     str[6] = 'x';
    if (mode & S_IROTH)     str[7] = 'r';               /* 其他人的三个属性 */
    if (mode & S_IWOTH)     str[8] = 'w';
    if (mode & S_IXOTH)     str[9] = 'x';
    if (S_ISDIR(mode))      str[0] = 'd', color = BLUE; /* 文件夹        */
    if (S_ISCHR(mode))      str[0] = 'c';               /* 字符设备      */
    if (S_ISBLK(mode))      str[0] = 'b';               /* 块设备        */

    /* 输出文件节点号 */
    if (myls_cmdline_cfg.print_inode)
        printf("%ld ", st->st_ino);

    if (myls_cmdline_cfg.print_blocks_size)
        printf("%3ld ", st->st_blocks);

    /* 如果需要输出详细信息，则依次输出文件权限，文件所有者，文件硬链接个数，文件大小，文件修改日期，文件名 */
    if (myls_cmdline_cfg.list_long) {
        /* 格式化文件修改时间 */
        struct tm *ptr;
        char time_buf[30];
        ptr = localtime(&st->st_ctime);
        strftime(time_buf, 30, "%b %d %G %H:%M", ptr);

        /* 获取文件所有者的用户名 */
        struct passwd *pwd = getpwuid(st->st_uid);

        printf("%s %s %2ld %5ld %s ", str, pwd->pw_name, st->st_nlink, st->st_size, time_buf);
        /* 颜色区分不同属性的文件/目录 */
        myfcolor(stdout, color);
        printf("%s\n", name);
    } else {
        /* 仅仅输出文件名 */
        myfcolor(stdout, color);
        printf("%s\n", name);
    }
    myfcolor(stdout, DEF_COLOR);
}

/* 输出当前扫描的目录的所有文件信息 */
void myls_dir(const char *dir)
{
    sort_files();
    for (int i = 0; i < myls.files_count; i++)
        myls_files(myls.files[i].name, &myls.files[i].st);
}

int main(int argc, char *argv[])
{
    myutils_set_name(argv[0]);
    default_cfg();
    parse_cmdline(argc, argv);
    myutils_set_color_enabled(myls_cmdline_cfg.use_color);

    getcwd(myls.cwd, sizeof(myls.cwd));

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
            myerror(errno, 0, "无法获取\"%s\"的属性", dir);
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
            myerror(errno, 0, "无法打开目录\"%s\"", dir);
            myls.error = 1;
            continue;
        }

        if (chdir(dir) < 0) {
            myerror(errno, 0, "无法进入到目录\"%s\"", dir);
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
    } while (myls_cmdline_cfg.files_count > n);

    free(myls.files);
    return myls.error;
}
