//require eprintf.h
//provide log.h
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
extern void log_init(void);
extern void log_finish(void);
extern void log_set_loglevel(int level);
extern void log_set_system_loglevel(const char *system, int level);
attribute_format_printf(3, 0) extern void vlogf(int level, const char *system, const char *fmt, va_list args);
attribute_format_printf(3, 4) extern void logf(int level, const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_emerg  (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_alert  (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_crit   (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_err    (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_warning(const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_notice (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_info   (const char *system, const char *fmt, ...);
attribute_format_printf(2, 3) extern void log_debug  (const char *system, const char *fmt, ...);
