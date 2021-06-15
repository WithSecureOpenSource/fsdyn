#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fsdyn/date.h>

static bool test_epoch_to_utc()
{
    time_t epoch;
    for (epoch = 1; epoch < 2145909600; epoch += 17113) {
        struct tm a, b;
        epoch_to_utc(epoch, &a);
        gmtime_r(&epoch, &b);
        char date[26];
        char gmtime_date[26];
        asctime_r(&a, date);
        asctime_r(&b, gmtime_date);
        if (strcmp(date, gmtime_date)) {
            fprintf(stderr,
                    "epoch: %.0f\n"
                    "  epoch_to_utc: %s"
                    "  gmtime:       %s",
                    (float) epoch, gmtime_date, date);
            return false;
        }
    }
    return true;
}

int main()
{
    if (!test_epoch_to_utc())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
