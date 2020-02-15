#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "abort.h"
#include "_printf.h"
#define VINE_PRINTF_H_INCLUDED
#include "log.h"

#define LEVEL_COLUMN_WIDTH 10
#define SYSTEM_COLUMN_WIDTH 12

static
int
global_loglevel = 8;

void
log_set_loglevel(int level)
{
	global_loglevel = level;
}

static
const char *
level_to_string(int level)
{
	switch (level) {
		case LOG_EMERG: return "EMERG";
		case LOG_ALERT: return "ALERT";
		case LOG_CRIT: return "CRIT";
		case LOG_ERR: return "ERR";
		case LOG_WARNING: return "WARNING";
		case LOG_NOTICE: return "NOTICE";
		case LOG_INFO: return "INFO";
		case LOG_DEBUG: return "DEBUG";
		default: abort_with_error("invalid log level %d (expected in [0,7])\n");
	}

	return NULL;
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
void
vlogf(int level, const char *system, const char *fmt, va_list args)
{
	if (level <= global_loglevel) {
		efprintf(stderr, "[%s] %s: ", level_to_string(level), system);
		evfprintf(stderr, fmt, args);
	}
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __GNUC__
__attribute__((format(printf, 3, 4)))
#endif
void
logf(int level, const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(level, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_emerg(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_EMERG, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_alert(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ALERT, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_crit(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_CRIT, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_err(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ERR, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_warning(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_WARNING, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_notice(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_NOTICE, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_info(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_INFO, system, fmt, args);
	va_end(args);
}

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void
log_debug(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_DEBUG, system, fmt, args);
	va_end(args);
}

