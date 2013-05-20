#ifndef _EMQ_LOG_INCLUDE
#define _EMQ_LOG_INCLUDE

#include <stdarg.h>

/** Log Level Part **/
#define DISABLE 4   /* Disable - system is unusable, and will aborted. For example: Memory Allocation failed. */
#define EMERG   3   /* Emergencies - system is unusable, and will aborted. For example: Memory Allocation failed. */
#define ISSUE   2   /* Some bad thing has occurs, but the situation is recoverable. For example: the thread poll expand failed. */
#define INFO    1   /* Some important event happens, need to record it. For example: */
#define DEBUG   0   /* Store all performance and command info. For example: the time of command execution */

void emqSetupLogging(int log_level, char* log_file_path);
void _emq_app_log(int level, int line, char *filename, const char *fmt, ...);

#define applog(LEVEL, ...) \
    _emq_app_log(LEVEL, __LINE__, __FILE__, __VA_ARGS__)

extern void emqSetupLogging(int log_level, char* log_file_path);
extern void _emq_app_log(int level, int line, char *filename, const char *fmt, ...);


#endif
