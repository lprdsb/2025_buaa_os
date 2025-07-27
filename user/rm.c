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
