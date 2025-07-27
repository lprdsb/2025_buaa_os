#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()#`\""

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
	// debugf("##################\n");

	if (strchr(SYMBOLS, *s)) {
		if (*s == '\"' || *s == '`') {
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
	while (*s && ((in_quot && !strchr("\"`", *s)) || !strchr(WHITESPACE SYMBOLS, *s))) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	// debugf("##################\n");
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		// np1 = np2 = *p1;
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}


void DEBUG(char *s){
	debugf("[debug]:#############[%08x] : %s #################\n", syscall_getenvid(), s);
}

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

int var_usage(void){
	printf("usage: declare [-xr] [NAME [=VALUE]]\n");
	return -1;
}

volatile int var_flag[256] = {0};
void print_vars(void) {
	// debugf("%d %d\n", vars[0].cnt, vars[1].cnt);
	for(int o = 0; o < 2; ++o){
		for (int i = 0; i < vars[o].cnt; i++) if(vars[o].var_list[i].ok) {
			// printf("declare ");
			// if (vars[o].var_list[i].r) printf("-r ");
			// if (o) printf("-x ");
			printf("%s=%s\n", vars[o].var_list[i].name, vars[o].var_list[i].val);
		}
	}
}

struct Var* find_var(const char *name) {
	// DEBUG("Asdasd");
			// DEBUG(name);
	for(int o = 0; o < 2; ++o){
		for (int i = 0; i < vars[o].cnt; i++) if(vars[o].var_list[i].ok){
			if (strcmp(vars[o].var_list[i].name, name) == 0){
				return &(vars[o].var_list[i]);
			}
		}
	}
	return 0;
}

int set_var(char *name, char *val) {
	// DEBUG(name);
	// if(val) DEBUG(val);
	// debugf("%d\n", var_flag['x']);
	struct Var* var = find_var(name);
	// debugf("%d\n", idx);
	// printk("%08x\n", curenv->env_parent_id);
	if (var) {
		// DEBUG("Asdasd");
		if (var->r) {
			printf("declare: %s is readonly\n", name);
			return -1;
		}
		if (val) {
			strcpy(var->val, val);
		}
		if (var_flag['r']) var->r = 1;
		var->ok = 1;
	} else {
		if (vars[0].cnt + vars[1].cnt >= 1024) {
			printf("declare: too many variables\n");
			return -1;
		}
		var = &vars[var_flag['x']].var_list[vars[var_flag['x']].cnt];
		strcpy(var->name, name);
		strcpy(var->val, val ? val : "");
		var->r = var_flag['r'];
		var->ok = 1;
		vars[var_flag['x']].cnt++;
	}
	return 0;
}


#define MAXARGS 128
u_int ipc_flag = 0;
u_int ipc_flag1 = 0;

char cmd_buf[1024];
char *pcmd_buf = cmd_buf;
int parsecmd(char **argv, int *rightpipe, int *leftenv) {
	ipc_flag = 0;
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		int p[2];
		// debugf("@@@@@@@@@  %d %d\n", argc, c);
		switch (c) {
			case 0:
				// debugf("@@@@@@@@  %d\n", argc);
				return argc;
			case 'w':
				// debugf("#### %s ####\n", t);
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
			case '`':
				gettoken(0, &t);
				argv[argc++] = t;
				// debugf("@@@ %x @@@1\n", t);
				// printf("%s###\n", t);
				pipe(p);
				int r1 = fork();
				if (r1 == 0) {
					dup(p[0], 0);
					close(p[0]);
					close(p[1]);
					readcmd(pcmd_buf);
					// DEBUG(pcmd_buf);
					argc--;
					// debugf("#%d#\n", strlen(pcmd_buf));
					if(pcmd_buf[0] && pcmd_buf[0] != ' '){
						argv[argc++] = pcmd_buf;
					}
					for(int i = 0; i < strlen(pcmd_buf) - 1; ++i){
						if(pcmd_buf[i] == ' ' && (pcmd_buf[i + 1] && pcmd_buf[i + 1] != ' ')){
							// debugf("%08x\n", pcmd_buf + (i + 1));
							argv[argc++] = pcmd_buf + (i + 1);
							// DEBUG(argv[argc - 1]);
						}
					}
					// argv[argc - 1] = pcmd_buf;
					while(*pcmd_buf){
						if(*pcmd_buf == ' ') *pcmd_buf = 0;
						pcmd_buf++;
					}
					pcmd_buf++;
					int tmp;
					u_int res = ipc_recv(&tmp, 0, 0);
					for(int i = 0; i < argc; ++i) DEBUG(argv[i]);
				}  else if (r1 > 0) {
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
				if (gettoken(0, &t) == 0) {
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
				if (*rightpipe == 0) {
					ipc_flag1 = 0;
					// DEBUG("asdasdasd");
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
					ipc_flag1 = 0;
					int tmp;
					u_int res = ipc_recv(&tmp, 0, 0);
					if(res != 0) return parsecmd(argv, rightpipe, leftenv);
					exit();
				} else if (r > 0) {
					ipc_flag = r;
					return argc;
				}
				break;
			case ';':
				*leftenv = fork();
				if (*leftenv == 0) {
					ipc_flag1 = 0;
					return argc;
				} else if (*leftenv > 0) {
					// u_int res = ipc_recv(&tmp, 0, 0);
					return parsecmd(argv, rightpipe, leftenv);
				}
				break;
			case '&':
				r = fork();
				if (r == 0) {
					ipc_flag1 = 0;
					return argc;
				} else if (r > 0) {
					// u_int res = ipc_recv(&tmp, 0, 0);
					return parsecmd(argv, rightpipe, leftenv);
				}
				break;
			case 2038:
				r = fork();
				if (r == 0) {
					ipc_flag1 = 0;
					int tmp;
					u_int res = ipc_recv(&tmp, 0, 0);
					if(res == 0) return parsecmd(argv, rightpipe, leftenv);
					exit();
				} else if (r > 0) {
					ipc_flag = r;
					return argc;
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
			// case '$':
			// 	gettoken(0, &t);
			// 	struct Var* var = find_var(t);
			// 	DEBUG(t);
			// 	if(var){
			// 		// DEBUG( var->val);
			// 		strcpy(pcmd_buf, var->val);
			// 		argv[argc++] = pcmd_buf;
			// 		pcmd_buf = cmd_buf + strlen(cmd_buf) + 1;
			// 	}
			// 	break;
		}
	}
	return argc;
}

void declare(int argc, char **argv){
	// DEBUG(argv[0]);
	// memset(var_flag, 0, sizeof var_flag);
	
	ARGBEGIN {
		default:
			try(var_usage());
		case 'x':
		case 'r':
			var_flag[(u_char)ARGC()]=1;
			break;
		}
	ARGEND
// 	// DEBUG(argv[0]);
	if (argc == 0) {
		print_vars();
		return;
	}
	for (int i = 0; i < argc; i++) {
		char *arg = argv[i];
		char *eq = strchr(arg, '=');
		if (eq) {
			*eq = '\0'; // 分离 name 和 value
			const char *name = arg;
			const char *val = eq + 1;
			try(set_var(name, val));
		} else {
			try(set_var(arg, NULL));
		}
	}
}

int unset(int argc, char **argv){
	
	if (argc == 0) {
		print_vars();
		return;
	}
	
	ARGBEGIN {
		default:
			try(var_usage());
		}
	ARGEND
	for (int i = 0; i < argc; i++) {
		struct Var* var = find_var(argv[i]);
		if(var) var->ok = 0;
	}
}

// void sendflag(u_int id, u_int res){
	
// }

void solargc(char **s){
	char *ps = *s;
	int len = strlen(ps);
	char tmp[512];
	int fl = 0;
	for(int i = 0; i < len; ++i){
		if((*s)[i] == '$'){
			fl = 1;
			break;
		}
	}
	if(fl){
		*s = pcmd_buf;
		while(*ps){
			if(*ps != '$'){
				*pcmd_buf = *ps;
				ps++;
				pcmd_buf++;
			}
			else{
				char *pt = tmp;
				*ps++;
				while(*ps && *ps != '/'){
					*pt = *ps;
					*ps++;
					*pt++;
				}
				*pt = 0;
				struct Var *var = find_var(tmp);
				if(var) strcpy(pcmd_buf, var->val);
				// DEBUG(var->val);
				pcmd_buf += strlen(pcmd_buf);
			}
		}
		*pcmd_buf++;
	}
}

char cwd_buf[MAXPATHLEN];
char prog_buf[MAXPATHLEN];
void runcmd(char *s) {
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int leftenv = 0;
	int argc = parsecmd(argv, &rightpipe, &leftenv);
	// debugf("@@@@@@@@1%d\n", argc);
	for(int i = 0; i < argc; ++i){
		solargc(&argv[i]);
	}
	if (leftenv) {
		wait(leftenv);
	}

	u_int res = 0;
	if (argc == 0) {
		res = -1;
	} else if (strcmp(argv[0], "cd") == 0) {
		if(argc > 2){
			printf("Too many args for cd command\n");
			res = -1;
		}
		else{
			envchdir(env->env_parent_id, argc > 1 ? argv[1] : "/");
		}
	} else if (strcmp(argv[0], "pwd") == 0) {
		u_int res = 0;
		if (argc != 1) {
			res = -1;
			printf("pwd: expected 0 arguments; got %d\n", argc - 1);
		}
		else printf("%s\n", getcwd(cwd_buf));
		// if(ipc_flag){
		// 	ipc_send(ipc_flag, res, 0, 0);
		// 	wait(ipc_flag);
		// }
		// if(ipc_flag1){
		// 	// DEBUG("asdasd");
		// 	// debugf("%08x\n", ipc_flag1);
		// 	ipc_send(ipc_flag1, 0, 0, 0);
		// 	wait(ipc_flag1);
		// }
		// return;
		close_all();
	} else if (strcmp(argv[0], "history") == 0) {
		if (argc != 1) {
			res = -1;
			printf("history: expected 0 arguments; got %d\n", argc - 1);
		}
		else{
			print_history();
			// if(ipc_flag){
			// 	ipc_send(ipc_flag, res, 0, 0);
			// 	wait(ipc_flag);
			// }
			// if(ipc_flag1){
			// 	// DEBUG("asdasd");
			// 	// debugf("%08x\n", ipc_flag1);
			// 	ipc_send(ipc_flag1, 0, 0, 0);
			// 	wait(ipc_flag1);
			// }
		}
		close_all();
		// return;
	}
	else if (strcmp(argv[0], "exit") == 0) {
		if (argc != 1) {
			res = -1;
			printf("exit: expected 0 arguments; got %d\n", argc - 1);
		}
		else{
			syscall_exitcmd(syscall_get_parentid());
			exit();
		}
	}
	else if(strcmp(argv[0], "declare") == 0){
		// DEBUG(argv[0]);
		declare(argc, argv);
	}
	else if(strcmp(argv[0], "unset") == 0){
		// DEBUG(argv[0]);
		unset(argc, argv);
	}
	else{
		argv[argc] = 0;
		int len = strlen(argv[0]);
		if(!strcmp(&argv[0][len - 2], ".b")) argv[0][len - 2] = 0;
		strcpy(prog_buf, argv[0]);
		strcpy(prog_buf + strlen(argv[0]), ".b");
		// DEBUG(prog_buf);
		// if(argc > 1) DEBUG(argv[1]);
		// DEBUG(argv[1]);
		int child = spawn(prog_buf, argv);
		
		close_all();
		if(child >= 0) {
			int tmp;
			// DEBUG(prog_buf);
			res = ipc_recv(&tmp, 0, 0);
			// DEBUG(prog_buf);
			wait(child);
			// DEBUG(prog_buf);
		} else {
			res = -1;
			debugf("spawn %s: %d\n", prog_buf, child);
		}
	}
	if(ipc_flag){
		// DEBUG("1");
		ipc_send(ipc_flag, res, 0, 0);
		wait(ipc_flag);
	}
	if (rightpipe) {
		// DEBUG("2");
		wait(rightpipe);
	}
	if(ipc_flag1){
		// DEBUG("3");
		// DEBUG("asdasd");
		// debugf("%08x\n", ipc_flag1);
		ipc_send(ipc_flag1, 0, 0, 0);
		wait(ipc_flag1);
	}
	// ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
	// exit();
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

void savecmd(char *s) {
	int fd;

	if ((fd = open("/.mos_history", O_CREAT | O_WRONLY)) < 0) {
		user_panic("open .mos_history, %d", fd);
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

	if ((fd = open("/.mos_history", O_CREAT | O_RDONLY)) < 0) {
		user_panic("open .mos_history, %d", fd);
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
	for (int j = len + 1; j > i; j--) {
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

int max(int x, int y){return x > y ? x : y;}

void print_history(){
	int fd;
	if ((fd = open("/.mos_history", O_CREAT | O_RDONLY)) < 0) {
		user_panic("open .mos_history, %d", fd);
	}
	char tmp[1024];
	for (int i = 0; i <= hsty_now; i++) {
		readline(fd, tmp, 1024);
		if(i >= max(0, hsty_now - 19)) printf("%s\n", tmp);
	}
	// printf("\f");
	
	// loadcmd_from_buf(p_cursor, buf, tmp);

	close(fd);
}

void print_cwd(){
	char tmp[1024];
	sprintf(tmp, "[shellid %08x]\e[34m%s\e[0m $ ", syscall_getenvid(), getcwd(cwd_buf));
	printf("%s", tmp);
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
			// ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 0, 0, 0);
			exit();
		}

		int len = strlen(buf);
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
	// ipc_send(envs[ENVX(syscall_getenvid())].env_parent_id, 1, 0, 0);
	exit();
}

void var_init(){
	// syscall_mem_map(0, &vars[0], 0, &vars[0], PTE_COW);
	if(!vars) vars = (struct Vars*)0x600000;
	syscall_mem_alloc(0, &vars[0], PTE_D | PTE_LIBRARY);
	if(*((u_int*)(vars + 1)) != 114514){
		// DEBUG("ASDSADASD");
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
	// memset(vars, 0, sizeof(struct Vars));
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
	// printf(":::::::::::::::::::::::::::::%d:::::::::::::::::::::::::::::::\n", envs[ENVX(syscall_getenvid())].exit);
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
	// debugf("@@@@@@@@@@ %08x @@@@@@@@\n", &vars[1]);
	// syscall_var_init(vars);
	var_init();
	// if(syscall_getenvid() == 0x2003){
	// 	*((int*)(vars + 1)) = 114514;
		// debugf("@@@@@@@@@@ %d @@@@@@@@\n", *((int*)(vars + 1)));
	// }
	// else{
	// 	debugf("@@@@@@@@@@ %d %08x @@@@@@@@\n", *((int*)(vars + 1)), envs[ENVX(syscall_getenvid())].pvar);
	// }
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
			printf("[shellid %08x]\e[34m%s\e[0m $ ", syscall_getenvid(), getcwd(cwd_buf));
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
			exit();
		} else {
			wait(r);
			
			// if(syscall_getenvid() == 0x2003){
			// 	debugf("@@@@@@@@@@ %d %08x @@@@@@@@\n", *((int*)(vars + 1)), envs[ENVX(syscall_getenvid())].pvar);
			// }
			if(envs[ENVX(syscall_getenvid())].exit == 1){
				ipc_send(syscall_get_parentid(), 0, 0, 0);
				exit();
			}
		}
	}
	return 0;
}