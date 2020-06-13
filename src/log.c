#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "abort.h"
#include "printf.h"
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

void
log_set_subsystem_loglevel(const char *subsystem, int level)
{
	(void)subsystem;
	(void)level;
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
		default: abort_with_error("invalid log level %d (expected in [0,7])\n", level);
	}

	return NULL;
}

void
vlogf(int level, const char *subsystem, const char *fmt, va_list args)
{
	if (level <= global_loglevel) {
		efprintf(stderr, "[%s] %s: ", level_to_string(level), subsystem);
		evfprintf(stderr, fmt, args);
	}
}

void
logf(int level, const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(level, subsystem, fmt, args);
	va_end(args);
}

void
log_emerg(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_EMERG, subsystem, fmt, args);
	va_end(args);
}

void
log_alert(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ALERT, subsystem, fmt, args);
	va_end(args);
}

void
log_crit(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_CRIT, subsystem, fmt, args);
	va_end(args);
}

void
log_err(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ERR, subsystem, fmt, args);
	va_end(args);
}

void
log_warning(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_WARNING, subsystem, fmt, args);
	va_end(args);
}

void
log_notice(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_NOTICE, subsystem, fmt, args);
	va_end(args);
}

void
log_info(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_INFO, subsystem, fmt, args);
	va_end(args);
}

void
log_debug(const char *subsystem, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_DEBUG, subsystem, fmt, args);
	va_end(args);
}

