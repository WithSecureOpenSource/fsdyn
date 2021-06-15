#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fsdyn/charstr.h>

static void dump_string(const char *str)
{
    size_t len = strlen(str);
    const char *end = str + len;
    char delim = '<';
    while (str != end) {
        int codepoint;
        str = charstr_decode_utf8_codepoint(str, end, &codepoint);
        fputc(delim, stderr);
        fprintf(stderr, "%x", codepoint);
        delim = ',';
    }
    fputc('>', stderr);
    fputc('\n', stderr);
}

static void dump_error(const char *name, const char *input, const char *actual,
                       const char *expected)
{
    fprintf(stderr, "Error: %s\n", name);
    fprintf(stderr, "Input: ");
    dump_string(input);
    fprintf(stderr, "Actual: ");
    dump_string(actual);
    fprintf(stderr, "Expected: ");
    dump_string(expected);
}

static bool verify_normalization(const char *str, const char *nfd_str,
                                 const char *nfc_str)
{
    char nfd_output[1000];
    const char *end = nfd_output + sizeof nfd_output;
    if (!charstr_unicode_decompose(str, NULL, nfd_output, end)) {
        fprintf(stderr, "Error: toNFD: %s\n", strerror(errno));
        fprintf(stderr, "Input: ");
        dump_string(str);
        return false;
    }
    if (strcmp(nfd_output, nfd_str)) {
        dump_error("toNFD", str, nfd_output, nfd_str);
        return false;
    }
    char nfc_output[1000];
    end = nfc_output + sizeof nfc_output;
    if (!charstr_unicode_recompose(nfd_output, NULL, nfc_output, end)) {
        fprintf(stderr, "Error: toNFC: %s\n", strerror(errno));
        fprintf(stderr, "Input: ");
        dump_string(nfd_output);
        return false;
    }
    if (strcmp(nfc_output, nfc_str)) {
        dump_error("toNFC", nfd_output, nfc_output, nfc_str);
        return false;
    }
    return true;
}

static bool test_normalization(char **strv)
{
    int i;
    for (i = 0; i < 3; i++)
        if (!verify_normalization(strv[i], strv[2], strv[1]))
            return false;
    for (; i < 5; i++)
        if (!verify_normalization(strv[i], strv[4], strv[3]))
            return false;
    return true;
}

static char *decode_column(const char *column)
{
    list_t *fields = charstr_split(column, ' ', -1);
    size_t size = list_size(fields) * 4 + 1;
    char *str = fsalloc(size);
    char *end = str + size;
    char *ptr = str;
    while (!list_empty(fields)) {
        char *field = (char *) list_pop_first(fields);
        long codepoint = strtol(field, NULL, 16);
        ptr = charstr_encode_utf8_codepoint(codepoint, ptr, end);
        fsfree(field);
    }
    *ptr = '\0';
    destroy_list(fields);
    return str;
}

static void decode_line(const char *line, char *strv[5])
{
    int i;
    char *columns[6];
    size_t n = charstr_split_into_array(line, ';', (char **) &columns, 5);
    assert(n == 5);
    for (i = 0; i < 5; i++) {
        strv[i] = decode_column(columns[i]);
        fsfree(columns[i]);
    }
    fsfree(columns[5]);
}

int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "r");
    assert(f);
    bool error = false;
    size_t buffer_size = 512;
    char *buffer = malloc(buffer_size);
    while (!error && getline(&buffer, &buffer_size, f) >= 0) {
        if (*buffer == '#' || *buffer == '@')
            continue;
        int i;
        char *strv[5];
        decode_line(buffer, strv);
        error = !test_normalization(strv);
        for (i = 0; i < 5; i++)
            fsfree(strv[i]);
    }
    free(buffer);
    fclose(f);
    if (error)
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
