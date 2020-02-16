#ifndef VINE_PRINTF_H_INCLUDED
#error "Must include printf.h before including log.h"
#endif
#ifdef VINE_LOG_H_INCLUDED
#error "May not include log.h more than once"
#endif
#define VINE_LOG_H_INCLUDED
enum log_level {
	LOG_EMERG,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG
};
extern void log_set_loglevel(int level);
extern void vlogf(int level, const char *system, const char *fmt, va_list args);
extern void logf(int level, const char *system, const char *fmt, ...);
extern void log_emerg  (const char *system, const char *fmt, ...);
extern void log_alert  (const char *system, const char *fmt, ...);
extern void log_crit   (const char *system, const char *fmt, ...);
extern void log_err    (const char *system, const char *fmt, ...);
extern void log_warning(const char *system, const char *fmt, ...);
extern void log_notice (const char *system, const char *fmt, ...);
extern void log_info   (const char *system, const char *fmt, ...);
extern void log_debug  (const char *system, const char *fmt, ...);
