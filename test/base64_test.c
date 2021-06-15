#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsdyn/base64.h>

static struct {
    const char *decoded, *encoded;
} data[] = {
    { "tytäryhtiöraportti", "dHl0w6RyeWh0acO2cmFwb3J0dGk=" },
    { "hälytysajoneuvo", "aMOkbHl0eXNham9uZXV2bw==" },
    { "012345678901234567890", "MDEyMzQ1Njc4OTAxMjM0NTY3ODkw" },
    { "", "" },
    { ".", "Lg==" },
    { 0 },
};

static bool test_encoding_length(void)
{
    int i;
    for (i = 0; data[i].decoded; i++) {
        size_t decoded_size = strlen(data[i].decoded);
        size_t count = base64_encode_buffer(data[i].decoded, decoded_size, NULL,
                                            0, -1, -1);
        if (count != strlen(data[i].encoded)) {
            fprintf(stderr, "Error: bad Base64 encoding size (1) for \"%s\"\n",
                    data[i].decoded);
            return false;
        }
        if (count != base64_encoding_size(decoded_size)) {
            fprintf(stderr, "Error: bad Base64 encoding size (2) for \"%s\"\n",
                    data[i].decoded);
            return false;
        }
    }
    return true;
}

static bool test_decoding_length(void)
{
    int i;
    for (i = 0; data[i].decoded; i++) {
        size_t count =
            base64_decode_buffer(data[i].encoded, strlen(data[i].encoded), NULL,
                                 0, -1, -1, false);
        if (count != strlen(data[i].decoded)) {
            fprintf(stderr, "Error: bad Base64 decoding size (1) for \"%s\"\n",
                    data[i].encoded);
            return false;
        }

        count =
            base64_decode_buffer(data[i].encoded, -1, NULL, 0, -1, -1, false);
        if (count != strlen(data[i].decoded)) {
            fprintf(stderr, "Error: bad Base64 decoding size (1) for \"%s\"\n",
                    data[i].encoded);
            return false;
        }
    }
    return true;
}

static bool test_truncated_encoding(void)
{
    int i;
    for (i = 0; data[i].decoded; i++) {
        size_t decoded_size = strlen(data[i].decoded);
        size_t encoded_size = strlen(data[i].encoded);
        char buffer[encoded_size + 10];
        int j;
        for (j = 0; j < encoded_size; j++) {
            memset(buffer, '^', encoded_size + 10);
            size_t count = base64_encode_buffer(data[i].decoded, decoded_size,
                                                buffer, j + 1, -1, -1);
            if (count != encoded_size) {
                fprintf(stderr,
                        "Error: bad Base64 encoding size (3) for \"%s\"\n",
                        data[i].decoded);
                return false;
            }
            if (buffer[j]) {
                fprintf(stderr, "Error: bad Base64 truncation (1) for \"%s\"\n",
                        data[i].decoded);
                return false;
            }
            if (memcmp(buffer, data[i].encoded, j)) {
                fprintf(stderr, "Error: bad Base64 truncation (2) for \"%s\"\n",
                        data[i].decoded);
                return false;
            }
        }
        memset(buffer, '^', encoded_size + 10);
        size_t count = base64_encode_buffer(data[i].decoded, decoded_size,
                                            buffer, encoded_size + 10, -1, -1);
        if (count != encoded_size) {
            fprintf(stderr, "Error: bad Base64 encoding size (4) for \"%s\"\n",
                    data[i].decoded);
            return false;
        }
        if (strcmp(buffer, data[i].encoded)) {
            fprintf(stderr, "Error: bad Base64 encoding for \"%s\"\n",
                    data[i].decoded);
            return false;
        }
    }
    return true;
}

static bool test_truncated_decoding(void)
{
    int i;
    for (i = 0; data[i].decoded; i++) {
        size_t decoded_size = strlen(data[i].decoded);
        size_t encoded_size = strlen(data[i].encoded);
        char buffer[decoded_size + 10];
        int j;
        for (j = 0; j < decoded_size; j++) {
            memset(buffer, '^', decoded_size + 10);
            size_t count = base64_decode_buffer(data[i].encoded, encoded_size,
                                                buffer, j, -1, -1, false);
            if (count != decoded_size) {
                fprintf(stderr,
                        "Error: bad Base64 decoding size (2) for \"%s\"\n",
                        data[i].encoded);
                return false;
            }
            if (buffer[j] != '^') {
                fprintf(stderr, "Error: bad Base64 truncation (3) for \"%s\"\n",
                        data[i].encoded);
                return false;
            }
            if (memcmp(buffer, data[i].decoded, j)) {
                fprintf(stderr, "Error: bad Base64 truncation (4) for \"%s\"\n",
                        data[i].encoded);
                return false;
            }
        }
        memset(buffer, '^', decoded_size + 10);
        size_t count =
            base64_decode_buffer(data[i].encoded, encoded_size, buffer,
                                 decoded_size + 10, -1, -1, false);
        if (count != decoded_size) {
            fprintf(stderr, "Error: bad Base64 decoding size (3) for \"%s\"\n",
                    data[i].encoded);
            return false;
        }
        if (memcmp(buffer, data[i].decoded, count)) {
            fprintf(stderr, "Error: bad Base64 decoding for \"%s\"\n",
                    data[i].encoded);
            return false;
        }
    }
    return true;
}

int main()
{
    if (!test_encoding_length())
        return EXIT_FAILURE;
    if (!test_decoding_length())
        return EXIT_FAILURE;
    if (!test_truncated_encoding())
        return EXIT_FAILURE;
    if (!test_truncated_decoding())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
