#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>
#include "emq_log.h"


typedef struct _emq_log_setting{
	char* log_file_path;
	int log_level;
}EmqLogSetting; /* The Log Will be used both at Master Runtime and Region Runtime. */

EmqLogSetting *emqLogSetting = NULL; /* Init */

void emqLoggingSetup(int log_level, char* log_file_path){
    emqLogSetting = malloc(sizeof(EmqLogSetting));
    emqLogSetting->log_level = log_level;
    if(log_file_path == NULL || strlen(log_file_path) == 0) {
        emqLogSetting->log_file_path = NULL;
    }else {
        emqLogSetting->log_file_path = malloc(strlen(log_file_path)+1);
        strncpy(emqLogSetting->log_file_path, log_file_path, strlen(log_file_path));
    }
}

void emqLoggintFree()
{
    if(emqLogSetting->log_file_path != NULL)
        free(emqLogSetting->log_file_path);

    free(emqLogSetting);
    emqLogSetting = NULL;

    return;
}

void _emq_app_log(int level, int line, char *filename, const char *fmt, ...){
    if(emqLogSetting == NULL){
        printf("Please Setup Logging first before using the logging function!\n");
        return;
    }
    va_list ap;
    FILE *fp;
    int fd;
    char *c = ".-*#$@";
    char buf[64];
    time_t now;
    struct timeval time1;

    if (level < emqLogSetting->log_level) return;

    fp = (emqLogSetting->log_file_path == NULL) ? stdout : fopen(emqLogSetting->log_file_path,"a");
    if (!fp) { printf("skyline in log file return\n");return; }

    va_start(ap, fmt);
    now = time(NULL);
    memset(buf, 0x00, sizeof(buf));
    strftime(buf,64,"%d %b %H:%M:%S",localtime(&now));
    /*Write to the log file */
    /* lock the file */
    fd = fileno(fp);
    lockf(fd, F_LOCK, 0l);
    fprintf(fp,"[%d] %s %s-%d %c ",(int)getpid(),buf, filename, line, c[level]);
    vfprintf(fp, fmt, ap);
    /* unlock */
    lockf(fd, F_ULOCK, 0l);
    /*Write to the stdout */
    printf("[%d] %s %s-%d %c ",(int)getpid(),buf, filename, line, c[level]);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
    fprintf(fp, "\n");
    fflush(fp);
    fflush(stdout);
    va_end(ap);

    if (emqLogSetting->log_file_path != NULL) fclose(fp);
}
