#include <time.h>

struct logcfg {
    int level;
    FILE *fp;
    const char *logfmt;
    const char *timefmt;
};

static int logcwd(FILE *fp)
{
}

static int logtime(FILE *fp, const logcfg_t *cfg)
{
    char tstr[1024];

}

static int logprefix(FILE *fp, const logcfg_t *cfg)
{
    int rv, pct;

    pct = 0;
    while (fmt) {
        if (pct) {
            rv = 0;
            switch (*fmt++) {
            case '%':
                fputc('%', fp);
                break;
            case 't':
                rv = logtime(fp, cfg);
                break;
            case 'd':
                rv = logcwd(fp);
                break;
            }

            if (rv < 0) {
                return -1;
            }

            pct = 0;
        } else {
        }
    }
}

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

int debug(const char *tag, const char *fmt);
int info(const char *tag, const char *fmt);
int warn(const char *tag, const char *fmt);
int error(const char *tag, const char *fmt);
int fatal(const char *tag, const char *fmt);

int log(int level, const char *tag, const char *fmt, ...);
int logv(int level, const char *tag, const char *fmt, va_list ap);
int islogged(int level, const char *tag);
