挑战性任务

![](img/111.png)







# 支持相对路径

每次运行到与相对路径有关的时候就将相对路径转为绝对路径

```c
char *getcwd(char *buf) {
	strcpy(buf, env->env_curpath);
	return buf;
}

void r2abs(char *abspath, const char *path) {
	char tmp[MAXPATHLEN];
	if (path && path[0] != '/') {
		getcwd(abspath);
		int len = strlen(abspath);
		abspath[len++] = '/';
		strcpy(abspath + len, path);
	} else {
		strcpy(abspath, path);	
	}
}
```

# 内建指令pwd

直接输出即可

```c
if (strcmp(argv[0], "pwd") == 0) {
		u_int res = 0;
		if (argc != 1) {
			res = -1;
			printf("pwd: expected 0 arguments; got %d\n", argc - 1);
		}
		else printf("%s\n", getcwd(cwd_buf));
		close_all();
}
```

# 环境变量管理

定义环境变量结构体，每个结构体占一页内存，一共两个结构体，分别表示环境变量和局部变量

```c
struct Var{
	char name[25];
	char val[25];
	int r;
	int ok;
};

struct Vars{
	u_int magic;
	int cnt;
	struct Var var_list[20];
	char v_pad[PAGE_SIZE - 4 * 2 - 20 * sizeof(struct Var)];
} __attribute__((aligned(4), packed)) *vars;
```

每次创建shell的时候将这环境变量对应的内存换成cow，这样即可保证子shell不影响父shell。

```c
void var_init(){
	if(!vars) vars = (struct Vars*)0x600000;
	syscall_mem_alloc(0, &vars[0], PTE_D | PTE_LIBRARY);
	if(*((u_int*)(vars + 1)) != 114514){
		syscall_mem_alloc(0, &vars[1], PTE_D | PTE_LIBRARY);
		*((u_int*)(vars + 1)) = 114514;
	}
	else{
		syscall_mem_map(0, &vars[1], 0, &vars[1], PTE_COW);
		for(int i = 0; i < PAGE_SIZE; ++i){
			*((char*)(vars + 1) + 1) = *((char*)(vars + 1) + 1);
		}
		syscall_mem_map(0, &vars[1], 0, &vars[1], PTE_D | PTE_LIBRARY);
	}
}
```

# 输入指令优化

spawn之前统一将.b去掉，执行时统一加上

```c
int len = strlen(argv[0]);
if(!strcmp(&argv[0][len - 2], ".b")) argv[0][len - 2] = 0;
strcpy(prog_buf, argv[0]);
strcpy(prog_buf + strlen(argv[0]), ".b");
```

# 快捷键

按要求模拟即可

```c
switch (ch) {
    case 0x01:
        while(cursor > 0){
            cursor--;
            printf("\b");
        }
        break;
    case 0x05:
        while(cursor < len){
            printf("%c", buf[cursor]);
            cursor++;
        }
        break;
    case 0x0B:
        printf("\33[F");
        print_cwd();
        for(int i = 0; i < cursor; ++i) printf("%c", buf[i]);
        for(int i = cursor; i <= len; ++i) printf(" ");
        for(int i = cursor; i <= len; ++i) printf("\b");
        buf[cursor] = '\0';
        break;
    case 0x15:
        // printf("\33[F");
        // print_cwd();
        for(int i = 0; i < cursor; ++i) printf("\b");
        for(int i = cursor; i < len; ++i) printf("%c", buf[i]);
        for(int i = 0; i <= len; ++i) printf(" ");
        for(int i = 0; i <= len; ++i) printf("\b");
        for(int i = cursor; i < len; ++i) printf("\b");
        char *s = buf;
        for(int i = cursor; i <= len; ++i){
            *s = buf[i];
            s++;
        }
        *s = '\0';
        cursor = 0;
        break;
    case 0x17:
        while(cursor > 0 && buf[cursor - 1] == ' '){
            buf[cursor - 1] = 0;
            printf("\b");
            printf(" ");
            printf("\b");
            cursor--;
        }
        while(cursor > 0 && buf[cursor - 1] != ' '){
            buf[cursor - 1] = 0;
            printf("\b");
            printf(" ");
            printf("\b");
            cursor--;
        }
        break;
    case '\b':
    case 0x7f:
        if (cursor > 0) {
            remove_char(buf, cursor - 1);
            cursor--;
        }
        break;
    case '\r':
    case '\n':
        return;
        break;
    case 0x1b: // read \e
        read(0, &ch, 1); // read [
        read(0, &ch, 1); // read A B C D for arrow keys
        switch(ch) {
            case 'A':	
                printf("\n[shellid %08x]\e[34m%s\e[0m $ ", syscall_getenvid(), getcwd(cwd_buf));
                if (hsty_now == hsty_num) {
                    strcpy(cmdbuf, buf);
                }
                hsty_now = hsty_now > 0 ? hsty_now - 1 : 0;
                loadcmd(&cursor, buf, hsty_now);
                break;
            case 'B':
                printf("\r[shellid %08x]\e[34m%s\e[0m $ ", syscall_getenvid(), getcwd(cwd_buf));
                hsty_now = hsty_now < hsty_num ? hsty_now + 1 : hsty_num;
                if (hsty_now == hsty_num) {
                    loadcmd_from_buf(&cursor, buf, cmdbuf);
                } else {
                    loadcmd(&cursor, buf, hsty_now);
                }
                break;
            case 'C':
                if (cursor < strlen(buf)) {
                    cursor++;
                } else {
                    printf("\b");
                }
                break;
            case 'D':
                if (cursor > 0) {
                    cursor--;
                } else {
                    printf(" ");
                }
                break;
            default:
                break;
        }
        break;
    default:
        if (insert_char(buf, cursor, ch) < 0) {
            goto err;
        }
        cursor++;
        break;
}
```

# 历史指令

按要求把历史指令存在文件里面即可

```c
void init_history() {
	int fd;

	if ((fd = open("/.mos_history", O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
		user_panic("open .mos_history, %d", fd);
	}

	char tmp[1024];
	hsty_num = 0;
	while (readline(fd, tmp, 1024) > 0) {
		hsty_num++;
	}

	hsty_now = hsty_num;

	close(fd);
}
```

# 实现注释功能 实现反引号 实现一行多指令 指令条件执行

实现注释功能，读到#时直接return

实现反引号，类似|管道，读到`直接开个管道把输出拉过来就行

实现一行多指令，类似|管道，不过不用开管道

指令条件执行，类似|管道，不过不用开管道，只需要把上一个程序的返回值拿过来判断即可。

```c
switch(c) {
    case 0:
        // debugf("@@@@@@@@  %d\n", argc);
        return argc;
    case 'w':
        // debugf("#### %s ####\n", t);
        if(argc >= MAXARGS) {
            debugf("too many arguments\n");
            exit();
        }
        argv[argc++] = t;
        break;
    case '<':
        if(gettoken(0, &t) != 'w') {
            debugf("syntax error: < not followed by word\n");
            exit();
        }
        // Open 't' for reading, dup it onto fd 0, and then close the original fd.
        /* Exercise 6.5: Your code here. (1/3) */
        fd = open(t, O_RDONLY);
        if(fd < 0) {
            debugf("failed to open '%s'\n", t);
            exit();
        }
        dup(fd, 0);
        close(fd);
        break;
    case '>':
        int tmp = gettoken(0, &t);
        if(tmp != 'w') {
            debugf("syntax error: > not followed by word\n");
            exit();
        }
        // Open 't' for writing, dup it onto fd 1, and then close the original fd.
        /* Exercise 6.5: Your code here. (2/3) */
        fd = open(t, O_WRONLY | O_CREAT | O_TRUNC);
        if(fd < 0) {
            debugf("failed to open '%s'\n", t);
            exit();
        }
        dup(fd, 1);
        close(fd);
        break;
    case 2062:
        if(gettoken(0, &t) != 'w') {
            debugf("syntax error: >> not followed by word\n");
            exit();
        }
        // printf("asdasdasdasd\n");
        fd = open(t, O_WRONLY | O_CREAT);
        int n;
        while((n = read(fd, buf, (long)sizeof buf)) > 0);
        struct Fd *f;
        f = (struct Fd *)INDEX2FD(fd);
        struct Filefd *filefd = (struct Filefd *)f;
        f->fd_offset = filefd->f_file.f_size;
        r = dup(fd, 1);
        close(fd);
        if(r < 0) {
            user_panic(">> redirection not implemented");
        }
        break;
    case '`':
        gettoken(0, &t);
        argv[argc++] = t;
        // debugf("@@@ %x @@@1\n", t);
        // printf("%s###\n", t);
        pipe(p);
        int r1 = fork();
        if(r1 == 0) {
            dup(p[0], 0);
            close(p[0]);
            close(p[1]);
            readcmd(pcmd_buf);
            // DEBUG(pcmd_buf);
            argc--;
            // debugf("#%d#\n", strlen(pcmd_buf));
            if(pcmd_buf[0] && pcmd_buf[0] != ' ') {
                argv[argc++] = pcmd_buf;
            }
            for(int i = 0; i < strlen(pcmd_buf) - 1; ++i) {
                if(pcmd_buf[i] == ' ' && (pcmd_buf[i + 1] && pcmd_buf[i + 1] != ' ')) {
                    // debugf("%08x\n", pcmd_buf + (i + 1));
                    argv[argc++] = pcmd_buf + (i + 1);
                    // DEBUG(argv[argc - 1]);
                }
            }
            // argv[argc - 1] = pcmd_buf;
            while(*pcmd_buf) {
                if(*pcmd_buf == ' ') *pcmd_buf = 0;
                pcmd_buf++;
            }
            pcmd_buf++;
            int tmp;
            u_int res = ipc_recv(&tmp, 0, 0);
            for(int i = 0; i < argc; ++i) DEBUG(argv[i]);
        }
        else if(r1 > 0) {
            dup(p[1], 1);
            close(p[1]);
            close(p[0]);
            strcpy(pcmd_buf, t);
            gettoken(pcmd_buf, 0);
            pcmd_buf += strlen(pcmd_buf) + 1;
            ipc_flag1 = r1;
            return parsecmd(argv, rightpipe, leftenv);
        }
        // argc--;
        if(gettoken(0, &t) == 0) {
            return argc;
        }
        break;
    case '|':
        /*
        * First, allocate a pipe.
        * Then fork, set '*rightpipe' to the returned child envid or zero.
        * The child runs the right side of the pipe:
        * - dup the read end of the pipe onto 0
        * - close the read end of the pipe
        * - close the write end of the pipe
        * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
        *   command line.
        * The parent runs the left side of the pipe:
        * - dup the write end of the pipe onto 1
        * - close the write end of the pipe
        * - close the read end of the pipe
        * - and 'return argc', to execute the left of the pipeline.
        */
        // int p[2];
        /* Exercise 6.5: Your code here. (3/3) */
        // DEBUG(argv[0]);
        pipe(p);
        *rightpipe = fork();
        if(*rightpipe == 0) {
            ipc_flag1 = 0;
            // DEBUG("asdasdasd");
            dup(p[0], 0);
            close(p[0]);
            close(p[1]);
            return parsecmd(argv, rightpipe, leftenv);
        }
        else if(*rightpipe > 0) {
            dup(p[1], 1);
            close(p[1]);
            close(p[0]);
            return argc;
        }
        break;
    case 20124:
        r = fork();
        if(r == 0) {
            ipc_flag1 = 0;
            int tmp;
            u_int res = ipc_recv(&tmp, 0, 0);
            if(res != 0) return parsecmd(argv, rightpipe, leftenv);
            exit();
        }
        else if(r > 0) {
            ipc_flag = r;
            return argc;
        }
        break;
    case ';':
        *leftenv = fork();
        if(*leftenv == 0) {
            ipc_flag1 = 0;
            return argc;
        }
        else if(*leftenv > 0) {
            // u_int res = ipc_recv(&tmp, 0, 0);
            return parsecmd(argv, rightpipe, leftenv);
        }
        break;
    case '&':
        r = fork();
        if(r == 0) {
            ipc_flag1 = 0;
            return argc;
        }
        else if(r > 0) {
            // u_int res = ipc_recv(&tmp, 0, 0);
            return parsecmd(argv, rightpipe, leftenv);
        }
        break;
    case 2038:
        r = fork();
        if(r == 0) {
            ipc_flag1 = 0;
            int tmp;
            u_int res = ipc_recv(&tmp, 0, 0);
            if(res == 0) return parsecmd(argv, rightpipe, leftenv);
            exit();
        }
        else if(r > 0) {
            ipc_flag = r;
            return argc;
        }
        break;
    case '\"':
        gettoken(0, &t);
        argv[argc++] = t;
        if(gettoken(0, &t) == 0) {
            return argc;
        }
        break;
    case '#':
        return argc;
}
```

# touch

按要求模拟即可

```c
#include <lib.h>

void usage(void) {
	printf("usage: touch file...\n");
	exit();
}

int main(int argc, char *argv[]) {
	int fd;
	if (argc < 2) {
		usage();
	}
	
	char *buf[512];
	for (int i = 1; i < argc; i++) {
		fr2abs(buf, argv[i]);
		// argv[i] = buf;
		// printf("%s\n", argv[i]);
		int fdnum;
		struct Filefd *fd;
		if ((fdnum = open(buf, O_CREAT)) >= 0) {
			close(fdnum);
			int r;
			if ((r = fd_lookup(fdnum, &fd)) < 0) {
				return r;
			}
			fd->f_file.f_type = FTYPE_REG;
		}
		else{
			printf("touch: cannot touch '%s': No such file or directory\n", argv[i]);
			return -1;
		}
	}
	
	return 0;
}
```

# mkdir

按要求模拟即可

```c
#include <lib.h>

void usage(void) {
	printf("usage: mkdir dir...\n");
	exit();
}

int flag[256];
int main(int argc, char *argv[]) {

	ARGBEGIN {
		case 'p':
			flag[(u_char)ARGC()]++;
			break;
		}
	ARGEND

	int r;
	int rt = 0;
	struct Stat stat_buf;
	if (argc < 1) {
		usage();
	}

	char *buf[512];
	for (int i = 0; i < argc; i++) {
		fr2abs(buf, argv[i]);
		// argv[i] = buf;
        // printf("%d####%s\n", i, argv[i]);
		if(flag['p']){
			if ((r = open(buf, O_FCREAT)) < 0) {
				return r;
			}
			close(r);
		}
		else{
			if (stat(buf, &stat_buf) >= 0) {
				printf("mkdir: cannot create directory '%s': File exists\n", argv[i]);
				rt = -1;
				continue;
			}
			if ((r = mkdir(buf)) < 0) {
				printf("mkdir: cannot create directory '%s': No such file or directory\n", argv[i]);
				rt = r;
			}
		}
	}

	return rt;
}

```

# rm

按要求模拟即可

```c
#include <lib.h>
int flag[256];

void usage(void) {
	printf("usage: rm file...\n");
	exit();
}

int main(int argc, char *argv[]) {
	ARGBEGIN {
		case 'r':
		case 'f':
			flag[(u_char)ARGC()]++;
			break;
	}
	ARGEND
	int rt = 0;
	int r;

	if (argc < 1) {
		usage();
	}
	char *buf[512];
	for (int i = 0; i < argc; i++) {
		fr2abs(buf, argv[i]);
		// argv[i] = buf;
		if (flag['r']) {
			if((r = remove(buf)) < 0){
				// printf("%d\n", r);
				if(r == -E_NOT_FOUND){
					if(!flag['f']) printf("rm: cannot remove '%s': No such file or directory\n", argv[i]);
				}
				// else if()
				// printf("fail to remove %s\n", argv[i]);
				rt = r;
			}
		}
		else{
			int fdnum;
			struct Filefd *fd;
			if ((fdnum = open(buf, O_RDONLY)) >= 0) {
				close(fd);
				if ((r = fd_lookup(fdnum, &fd)) < 0) {
					return r;
				}
				if(fd->f_file.f_type == FTYPE_DIR){
					printf("rm: cannot remove '%s': Is a directory\n", argv[i]);
				}
				else{
					if (r = remove(buf) < 0) {
						rt = r;
					}
				}
			}
			else{
				printf("rm: cannot remove '%s': No such file or directory\n", argv[i]);
				rt = -1;
			}
		}
	}

	return rt;
}

```

