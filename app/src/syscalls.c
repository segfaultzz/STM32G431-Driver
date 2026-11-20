/* newlib hooks */
#include <sys/stat.h>
#include <reent.h>
#include "FreeRTOS.h"
#include "task.h"

/* printf() output goes here: route it to your UART driver */
int _write(int fd, const char *buf, int len)
{
    (void)fd;
    (void)buf;
    return len;
}

/* newlib's malloc is not thread safe on its own: stop task switching while it runs */
void __malloc_lock(struct _reent *r)
{
    (void)r;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        vTaskSuspendAll();
    }
}

void __malloc_unlock(struct _reent *r)
{
    (void)r;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        (void)xTaskResumeAll();
    }
}

int _close(int fd)                       { (void)fd; return -1; }
int _read(int fd, char *p, int n)        { (void)fd; (void)p; (void)n; return 0; }
int _lseek(int fd, int off, int whence)  { (void)fd; (void)off; (void)whence; return 0; }
int _isatty(int fd)                      { (void)fd; return 1; }
int _fstat(int fd, struct stat *st)      { (void)fd; st->st_mode = S_IFCHR; return 0; }
