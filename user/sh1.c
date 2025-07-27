#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()#`"

char buf[1024];

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	static int in_quot = 0;
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		in_quot = 0;
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		if (*s == '\"') {
			in_quot = !in_quot;
		}
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if(t == '>' && *s == '>'){
			*s++ = 0;
			*p2 = s;
			return 2062;
		}
		if(t == '&' && *s == '&'){
			*s++ = 0;
			*p2 = s;
			return 2038;
		}
		if(t == '|' && *s == '|'){
			*s++ = 0;
			*p2 = s;
			return 20124;
		}
		return t;
	}

	*p1 = s;
	while (*s && ((in_quot && !strchr("\"", *s)) || !strchr(WHITESPACE SYMBOLS, *s))) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128
int ipc_flag = 0;

int parsecmd(char **argv, int *rightpipe, int *leftenv) {
	ipc_flag = 0;
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			/* Exercise 6.5: Your code here. (1/3) */
			fd = open(t, O_RDONLY);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 0);
			close(fd);
			break;
		case '>':
			int tmp = gettoken(0, &t);
			if (tmp != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, dup it onto fd 1, and then close the original fd.
			/* Exercise 6.5: Your code here. (2/3) */
			fd = open(t, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 1);
			close(fd);
			break;
		case 2062:
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: >> not followed by word\n");
				exit();
			}
			// printf("asdasdasdasd\n");
			fd = open(t, O_WRONLY | O_CREAT);
			int n;
			while ((n = read(fd, buf, (long)sizeof buf)) > 0);
			struct Fd *f;
			f=(struct Fd *)INDEX2FD(fd);
			struct Filefd *filefd=(struct Filefd *)f;
			f->fd_offset=filefd->f_file.f_size;
			r=dup(fd, 1);
			close(fd);
			if (r < 0) {
				user_panic(">> redirection not implemented");
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
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			pipe(p);
			*rightpipe = fork();
			if (*rightpipe == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe, leftenv);
			}  else if (*rightpipe > 0) {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		case 20124:
			r = fork();
			if (r == 0) {
				ipc_flag = 1;
				return argc;
			} else if (r > 0) {
				int tmp;
				u_int res = ipc_recv(&tmp, 0, 0);
				wait(r);
				// debugf("######### && ##########\n");
				if(res != 0) return parsecmd(argv, rightpipe, leftenv);
				exit();
			}
			break;
		case ';':
			*leftenv = fork();
			if (*leftenv == 0) {
				return argc;
			} else if (*leftenv > 0) {
				return parsecmd(argv, rightpipe, leftenv);
			}
			break;
		case '&':
			r = fork();
			if (r == 0) {
				return argc;
			} else if (r > 0) {
				return parsecmd(argv, rightpipe, leftenv);
			}
			break;
		case 2038:
			r = fork();
			if (r == 0) {
				ipc_flag = 1;
				return argc;
			} else if (r > 0) {
				int tmp;
				u_int res = ipc_recv(&tmp, 0, 0);
				wait(r);
				// debugf("######### && ##########\n");
				if(res == 0) return parsecmd(argv, rightpipe, leftenv);
				exit();
			}
			break;
		case '\"':
			gettoken(0, &t);
			argv[argc++] = t;
			if (gettoken(0, &t) == 0) {
				return argc;
			}
			break;
		case '#':
			return argc;
		}
	}

	return argc;
}

char cwd_buf[MAXPATHLEN];
char prog_buf[MAXPATHLEN];
void runcmd(char *s) {
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int leftenv = 0;
	int argc = parsecmd(argv, &rightpipe, &leftenv);
	if (leftenv) {
		wait(leftenv);
	}

	if (argc == 0) {
		return;
	} else if (strcmp(argv[0], "cd") == 0) {
		if (envchdir(env->env_parent_id, argc > 1 ? argv[1] : "/") < 0) {
			printf("path %s not exists\n", argv[1]);
		}
		return;
	} else if (strcmp(argv[0], "pwd") == 0) {
		if (argc != 1) {
			printf("pwd: expected 0 arguments; got %d\n", argc - 1);
		}
		else printf("%s\n", getcwd(cwd_buf));
		return;
	} else if (strcmp(argv[0], "exit") == 0) {
		if (argc != 1) {
			printf("exit: expected 0 arguments; got %d\n", argc - 1);
		}
		ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 1, 0, 0);
		exit();
	}
	argv[argc] = 0;
	int len = strlen(argv[0]);
	if(!strcmp(&argv[0][len - 2], ".b")) argv[0][len - 2] = 0;
	strcpy(prog_buf, argv[0]);
	strcpy(prog_buf + strlen(argv[0]), ".b");
	int child = spawn(prog_buf, argv);
	// printf("########## %d ########1\n", child);
	close_all();
	// if(child){
	// 	printf("asdasdasds\n");
	// }
	// printf("######### %d #########2\n", child);
	if (child >= 0) {
		int tmp;
		u_int res = ipc_recv(&tmp, 0, 0);
		wait(child);
		if(ipc_flag) ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, res, 0, 0);
	} else {
		debugf("spawn %s: %d\n", prog_buf, child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	// printf("######### %d #########3\n", child);
	ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
	exit();
}

int readline(int fd, char *buf, int n) {
	int i = 0;
	while (readn(fd, buf + i, 1) == 1) {
		if (i >= n)
			return n;
		if (buf[i] == '\n')
			break;
		i++;
	}
	buf[i] = '\0';
	return i;
}

int hsty_num;
int hsty_now;
char cmdbuf[1024];

void init_history() {
	int fd;

	if ((fd = open("/.mosh_history", O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}

	char tmp[1024];
	hsty_num = 0;
	while (readline(fd, tmp, 1024) > 0) {
		hsty_num++;
	}

	hsty_now = hsty_num;

	close(fd);
}

void savecmd(char *s) {
	int fd;

	if ((fd = open("/.mosh_history", O_CREAT | O_WRONLY)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}

	struct Stat stat_buf;
	fstat(fd, &stat_buf);

	seek(fd, stat_buf.st_size);

	int len = strlen(s);
	s[len] = '\n';

	write(fd, s, len + 1);
	
	s[len] = '\0';

	hsty_num++;
	hsty_now = hsty_num;
	
	close(fd);
}

void loadcmd_from_buf(int *p_cursor, char *dst, char *from) {
	int buf_len = strlen(dst);
	int cursor = *p_cursor;

	for (int i = 0; i < buf_len; i++)
		printf(" ");
	for (int i = 0; i < buf_len; i++)
		printf("\b");

	memset(dst, 0, 1024);
	strcpy(dst, from);

	printf("%s", dst);
	*p_cursor = strlen(dst);
}

void loadcmd(int *p_cursor, char *buf, int no) {
	int fd;

	if ((fd = open("/.mosh_history", O_CREAT | O_RDONLY)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}
	
	char tmp[1024];
	for (int i = 0; i <= no; i++) {
		readline(fd, tmp, 1024);
	}
	
	loadcmd_from_buf(p_cursor, buf, tmp);

	close(fd);
}

int insert_char(char *buf, int i, char ch) {
	int len = strlen(buf);
	if (len + i >= 1024) {
		return -1;
	}
	for (int j = len; j > i; j--) {
		buf[j] = buf[j - 1];
	}
	buf[i] = ch;

	len++;
	for (int j = i + 1; j < len; j++) {
		printf("%c", buf[j]);
	}
	for (int j = i + 1; j < len; j++) {
		printf("\b");
	}
	
	return 0;
}

void remove_char(char *buf, int i) {
	if (i < 0) {
		return;
	}
	for (int j = i; buf[j]; j++) {
		buf[j] = buf[j + 1];
	}

	printf("\b");
	for (int j = i; buf[j]; j++) {
		printf("%c", buf[j]);
	}
	printf(" \b");
	for (int j = i; buf[j]; j++) {
		printf("\b");
	}
}


void printcwd(){
	printf("\n[shellid %08x]\e[34m%s\e[0m $ ", syscall_getenvid(), getcwd(cwd_buf));
}

void readcmd(char *buf) {
	int r;
	int cursor = 0;
	char ch;

	memset(buf, 0, 1024);
	
	while (1) {	
		if ((r = read(0, &ch, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
			exit();
		}

		switch (ch) {
			case 0x01:
				// printf("asdasda\n");
				if (cursor > 0) {
					cursor--;
				} else {
					printf(" ");
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
						printcwd();
						if (hsty_now == hsty_num) {
							strcpy(cmdbuf, buf);
						}
						hsty_now = hsty_now > 0 ? hsty_now - 1 : 0;
						loadcmd(&cursor, buf, hsty_now);
						break;
					case 'B':
						printcwd();
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
	}
	return;

err:
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}


void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 1, 0, 0);
	exit();
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}

	init_history();

	for (;;) {
		if (interactive) {
			printcwd();
		}
		readcmd(buf);

		if (buf[0] != '\0') {
			savecmd(buf);
		}

		if (buf[0] == '#') {
			continue;
		}		
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
			exit();
		} else {
			int tmp;
			u_int res = ipc_recv(&tmp, 0, 0);
			wait(r);
			if(res == 1){
				ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
				exit();
			}
		}
	}
	return 0;
}