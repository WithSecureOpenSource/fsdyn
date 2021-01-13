#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fsdyn/charstr.h>

static bool test_punycode_encoding(void)
{
    struct {
        const char *input, *output;
    } data[] = {
        { "你好你好", "xn--6qqa088eba" },
        { "hyvää.yötä", "xn--hyv-slaa.xn--yt-wia4e" },
        { "hyvää.yötä.", "xn--hyv-slaa.xn--yt-wia4e." },
        { "ä.ö", "xn--4ca.xn--nda" },
        { "Ä.Ö.", "xn--4ca.xn--nda." },
        { NULL }
    };
    for (int i; data[i].input; i++) {
        char *encoding = charstr_idna_encode(data[i].input);
        if (!encoding)
            return false;
        if (strcmp(encoding, data[i].output)) {
            fsfree(encoding);
            return false;
        }
        fsfree(encoding);
    }
    return true;
}

int main()
{
    if (!test_punycode_encoding())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
