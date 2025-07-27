#include <lib.h>
#include <print.h>

struct print_ctx {
	int fd;
	int ret;
};

struct sprint_ctx {
	char *s;
	int ret;
};

static void print_output(void *data, const char *s, size_t l) {
	struct print_ctx *ctx = (struct print_ctx *)data;
	if (ctx->ret < 0) {
		return;
	}
	int r = write(ctx->fd, s, l);
	if (r < 0) {
		ctx->ret = r;
	} else {
		ctx->ret += r;
	}
}

static void sprint_output(char *data, const char *s, size_t l) {
	struct sprint_ctx *ctx = (struct sprint_ctx *)data;
	for(int i = 1; i <= l; ++i){
		*ctx->s = *s;
		ctx->s++;
		s++;
		ctx->ret++;
	}
}

static int vfprintf(int fd, const char *fmt, va_list ap) {
	struct print_ctx ctx;
	ctx.fd = fd;
	ctx.ret = 0;
	vprintfmt(print_output, &ctx, fmt, ap);
	return ctx.ret;
}

static int svfprintf(char *s, const char *fmt, va_list ap) {
	struct sprint_ctx ctx;
	ctx.s = s;
	ctx.ret = 0;
	vprintfmt(sprint_output, &ctx, fmt, ap);
	return ctx.ret;
}

// static int vfprintf(int fd, const char *fmt, va_list ap) {
// 	struct print_ctx ctx;
// 	ctx.fd = fd;
// 	ctx.ret = 0;
// 	vprintfmt(print_output, &ctx, fmt, ap);
// 	return ctx.ret;
// }

int fprintf(int fd, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int r = vfprintf(fd, fmt, ap);
	va_end(ap);
	return r;
}

int printf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int r = vfprintf(1, fmt, ap);
	va_end(ap);
	return r;
}

int sprintf(char *s, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int r = svfprintf(s, fmt, ap);
	va_end(ap);
	return r;
}
