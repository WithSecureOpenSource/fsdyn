#ifndef __FSDYN_DATE__
#define __FSDYN_DATE__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Break epoch seconds down to date and time components. A simplified,
 * signal-safe replacement for gmtime_r(3).
 *
 * Note: The value of tm->tm_yday after a call to epoch_to_utc() is
 * unspecified. */
struct tm *epoch_to_utc(time_t epoch, struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif
