#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "abort.h"
#include "printf.h"
#include "log.h"
#include "alloc.h"
#include "hash.h"
#include "object.h"
#include "table.h"

#define LEVEL_COLUMN_WIDTH 10
#define SYSTEM_COLUMN_WIDTH 12
#define SYSTEM_TABLE_SIZE 16

static
int
g_loglevel = 8;

static
struct table
g_system_loglevels = {0};

void
log_set_loglevel(int level)
{
	g_loglevel = level;
}

void
log_set_system_loglevel(const char *system, int level)
{
	struct tkey k_system = table_key_cstr(system);

	if (g_system_loglevels.size == 0) {
		table_init(&g_system_loglevels, &sys_alloc, SYSTEM_TABLE_SIZE);
	}

	table_set_int(&g_system_loglevels, k_system, level);
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
		default: abort_with_error("invalid log level %d\n", level);
	}

	return NULL;
}

void
vlogf(int level, const char *system, const char *fmt, va_list args)
{
	int err, system_max;

	if (level > g_loglevel) {
		return;
	}

	err = table_try_get_int(&g_system_loglevels, table_key_cstr(system), &system_max);
	if (err == -1) {
		/* key missing - use global loglevel */
		if (level > g_loglevel)
			return;
	} else if (err == 0) {
		/* key present and correct type, use system loglevel */
		if (level > system_max)
			return;
	} else {
		/* key present but incorrect type, abort */
		abort_with_error("invalid log level type %d\n", err);
	}

	efprintf(stderr, "[%s] %s: ", level_to_string(level), system);
	evfprintf(stderr, fmt, args);
}

void
logf(int level, const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(level, system, fmt, args);
	va_end(args);
}

void
log_emerg(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_EMERG, system, fmt, args);
	va_end(args);
}

void
log_alert(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ALERT, system, fmt, args);
	va_end(args);
}

void
log_crit(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_CRIT, system, fmt, args);
	va_end(args);
}

void
log_err(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_ERR, system, fmt, args);
	va_end(args);
}

void
log_warning(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_WARNING, system, fmt, args);
	va_end(args);
}

void
log_notice(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_NOTICE, system, fmt, args);
	va_end(args);
}

void
log_info(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_INFO, system, fmt, args);
	va_end(args);
}

void
log_debug(const char *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(LOG_DEBUG, system, fmt, args);
	va_end(args);
}

