#ifndef __UTIL_H_INCLUDED__
#define __UTIL_H_INCLUDED__

#include <stdint.h>

/* Messaging */

enum test_loglevel {
	TEST_LOGLEVEL_ERR = 0,
	TEST_LOGLEVEL_WARN,
	TEST_LOGLEVEL_DEBUG
};

extern enum test_loglevel test_loglevel;
static inline void test_set_loglevel(enum test_loglevel level)
{
	test_loglevel = level;
}

#define pr_level(level, fmt, args...) do {	\
	if (test_loglevel >= level) {	\
		fprintf(stdout, fmt, ##args);	\
	}					\
} while (0)

#define pr_err(fmt, args...) pr_level(TEST_LOGLEVEL_ERR, fmt, ##args)
#define pr_warn(fmt, args...) pr_level(TEST_LOGLEVEL_WARN, fmt, ##args)
#define pr_debug(fmt, args...) pr_level(TEST_LOGLEVEL_DEBUG, fmt, ##args)


#define _OKNG(verb, jump, cond, fmt, args...) do {	\
	if (cond) {					\
		if (verb)				\
			printf("[ OK ] " fmt, ##args);	\
	} else {					\
		printf("[ NG ] " fmt, ##args);		\
		if (jump) {				\
			ret = -1;			\
			goto out;			\
		}					\
	}						\
} while (0)

#define OKNG(args...) _OKNG(1, 1, ##args)
#define NG(args...) _OKNG(0, 1, ##args)
#define OKNGNOJUMP(args...) _OKNG(1, 0, ##args)

/* Time */
#define DIFFUSEC(end, start) ((end.tv_sec - start.tv_sec) * 1000000UL + (end.tv_usec - start.tv_usec))
#define DIFFNSEC(end, start) ((end.tv_sec - start.tv_sec) * 1000000000UL + (end.tv_nsec - start.tv_nsec))
#define TIMER_KIND CLOCK_MONOTONIC_RAW /* CLOCK_THREAD_CPUTIME_ID */

/* Calculation emulation */
void fwq_init();
void fwq(long delay_cyc);

/* CPU location */
int print_cpu_last_executed_on();

#endif
