#include <assert.h>
#include <errno.h>

#include "charstr.h"
#include "fsdyn_version.h"

enum {
    MAX_CC_SEQ_LENGTH = 100,
};

typedef enum {
    RECOMPOSER_INIT,
    RECOMPOSER_STARTED,
    RECOMPOSER_DELIVERING,
    RECOMPOSER_DELIVERING_TERMINATED,
    RECOMPOSER_TERMINATED
} recomposer_state_t;

/* A generic recomposer. Keep calling recomposer_feed() until it
 * returns a nonnegative codepoint. Then keep calling
 * recomposer_read() until it returns a negative value. Then start
 * calling recomposer_feed() again. When the input is exhausted, call
 * recomposer_terminate(), and if recomposer_terminate() returns a
 * nonnegative codepoints, call recomposer_read() until it returns a
 * negative value.
 *
 * Negative return values are accompanied with an errno value. EAGAIN
 * means everything is ok. 0 means the output stream reached a
 * normal termination. Other values (notably EILSEQ) indicate a fatal
 * error. */
typedef struct {
    recomposer_state_t state;
    int starter;
    int ccs[MAX_CC_SEQ_LENGTH];
    int wr, rd;
} recomposer_t;

static void init_recomposer(recomposer_t *recomposer)
{
    recomposer->state = RECOMPOSER_INIT;
}

extern int _charstr_unicode_primary_composite(int starter, int cc);

static bool recomposer_blocked(int seq[], int count, int ccc)
{
    int j;
    for (j = 0; j < count; j++)
        if (charstr_unicode_canonical_combining_class(seq[j]) >= ccc)
            return true;
    return false;
}

static int recomposer_feed(recomposer_t *recomposer, int codepoint)
{
    switch (recomposer->state) {
        case RECOMPOSER_INIT:
            if (charstr_unicode_canonical_combining_class(codepoint))
                return codepoint;
            recomposer->state = RECOMPOSER_STARTED;
            recomposer->starter = codepoint;
            recomposer->wr = 0;
            errno = EAGAIN;
            return -1;
        case RECOMPOSER_STARTED: {
            int starter;
            int ccc = charstr_unicode_canonical_combining_class(codepoint);
            if (!recomposer_blocked(recomposer->ccs, recomposer->wr, ccc)) {
                starter = recomposer->starter;
                int p = _charstr_unicode_primary_composite(starter, codepoint);
                if (p >= 0) {
                    recomposer->starter = p;
                    errno = EAGAIN;
                    return -1;
                }
            }
            if (ccc) {
                if (recomposer->wr >= MAX_CC_SEQ_LENGTH) {
                    errno = ENOBUFS;
                    return -1;
                }
                recomposer->ccs[recomposer->wr++] = codepoint;
                errno = EAGAIN;
                return -1;
            }
            starter = recomposer->starter;
            recomposer->starter = codepoint;
            recomposer->state = RECOMPOSER_DELIVERING;
            recomposer->rd = 0;
            return starter;
        }
        default:
            assert(false);
    }
}

static int recomposer_terminate(recomposer_t *recomposer)
{
    switch (recomposer->state) {
        case RECOMPOSER_INIT:
            recomposer->state = RECOMPOSER_TERMINATED;
            errno = 0;
            return -1;
        case RECOMPOSER_STARTED:
            recomposer->state = RECOMPOSER_DELIVERING_TERMINATED;
            recomposer->rd = 0;
            return recomposer->starter;
        default:
            assert(false);
    }
}

static int recomposer_read(recomposer_t *recomposer)
{
    switch (recomposer->state) {
        case RECOMPOSER_INIT:
        case RECOMPOSER_STARTED:
            errno = EAGAIN;
            return -1;
        case RECOMPOSER_DELIVERING:
            if (recomposer->rd >= recomposer->wr) {
                recomposer->state = RECOMPOSER_STARTED;
                recomposer->wr = 0;
                errno = EAGAIN;
                return -1;
            }
            return recomposer->ccs[recomposer->rd++];
        case RECOMPOSER_DELIVERING_TERMINATED:
            if (recomposer->rd >= recomposer->wr) {
                recomposer->state = RECOMPOSER_TERMINATED;
                errno = 0;
                return -1;
            }
            return recomposer->ccs[recomposer->rd++];
        default:
            errno = 0;
            return -1;
    }
}

typedef enum {
    HANGUL_RECOMPOSER_INIT,
    HANGUL_RECOMPOSER_STARTED,
    HANGUL_RECOMPOSER_TERMINATED
} hangul_recomposer_state_t;

/* A Hangul recomposer. Keep calling hangul_recomposer_feed(). It
 * returns the next codepoint, or a negative value if it needs more
 * input. When the input is exhausted, call
 * hangul_recomposer_terminate(). It returns the final codepoint, or a
 * negative value if there is no more codepoint to output.
 *
 * errno is not used. */
typedef struct {
    hangul_recomposer_state_t state;
    int next;
} hangul_recomposer_t;

static void init_hangul_recomposer(hangul_recomposer_t *recomposer)
{
    recomposer->state = HANGUL_RECOMPOSER_INIT;
}

static int hangul_recomposer_feed(hangul_recomposer_t *recomposer,
                                  int codepoint)
{
    enum {
        S_BASE = 0xAC00,
        L_BASE = 0x1100,
        V_BASE = 0x1161,
        T_BASE = 0x11A7,
        L_COUNT = 19,
        V_COUNT = 21,
        T_COUNT = 28,
        N_COUNT = V_COUNT * T_COUNT,
        S_COUNT = L_COUNT * N_COUNT
    };
    switch (recomposer->state) {
        case HANGUL_RECOMPOSER_INIT:
            recomposer->state = HANGUL_RECOMPOSER_STARTED;
            recomposer->next = codepoint;
            return -1;
        case HANGUL_RECOMPOSER_STARTED: {
            int li = recomposer->next - L_BASE;
            if (li >= 0 && li < L_COUNT) {
                int vi = codepoint - V_BASE;
                if (vi >= 0 && vi < V_COUNT) {
                    recomposer->next = S_BASE + (li * V_COUNT + vi) * T_COUNT;
                    return -1;
                }
            }
            int si = recomposer->next - S_BASE;
            if (si >= 0 && si < S_COUNT && !(si % T_COUNT)) {
                int ti = codepoint - T_BASE;
                if (ti > 0 && ti < T_COUNT) {
                    recomposer->next += ti;
                    return -1;
                }
            }
            int next = recomposer->next;
            recomposer->next = codepoint;
            return next;
        }
        default:
            assert(false);
    }
}

static int hangul_recomposer_terminate(hangul_recomposer_t *recomposer)
{
    switch (recomposer->state) {
        case HANGUL_RECOMPOSER_INIT:
            recomposer->state = HANGUL_RECOMPOSER_TERMINATED;
            return -1;
        case HANGUL_RECOMPOSER_STARTED:
            recomposer->state = HANGUL_RECOMPOSER_TERMINATED;
            return recomposer->next;
        default:
            assert(false);
    }
}

char *charstr_unicode_recompose(const char *s, const char *end, char output[],
                                const char *output_end)
{
    char *q = output;
    recomposer_t recomposer;
    hangul_recomposer_t hangul_recomposer;
    init_recomposer(&recomposer);
    init_hangul_recomposer(&hangul_recomposer);
    int codepoint;
    while (s != end && *s) {
        s = charstr_decode_utf8_codepoint(s, end, &codepoint);
        if (!s) {
            errno = EILSEQ;
            return NULL;
        }
        codepoint = recomposer_feed(&recomposer, codepoint);
        while (codepoint >= 0) {
            codepoint = hangul_recomposer_feed(&hangul_recomposer, codepoint);
            if (codepoint >= 0) {
                q = charstr_encode_utf8_codepoint(codepoint, q, output_end);
                if (!q) {
                    errno = EOVERFLOW;
                    return NULL;
                }
            }
            codepoint = recomposer_read(&recomposer);
        }
        if (errno != EAGAIN)
            return NULL;
    }
    codepoint = recomposer_terminate(&recomposer);
    while (codepoint >= 0) {
        codepoint = hangul_recomposer_feed(&hangul_recomposer, codepoint);
        if (codepoint >= 0) {
            q = charstr_encode_utf8_codepoint(codepoint, q, output_end);
            if (!q) {
                errno = EOVERFLOW;
                return NULL;
            }
        }
        codepoint = recomposer_read(&recomposer);
    }
    if (errno != 0)
        return NULL;
    codepoint = hangul_recomposer_terminate(&hangul_recomposer);
    if (codepoint >= 0) {
        q = charstr_encode_utf8_codepoint(codepoint, q, output_end);
        if (!q) {
            errno = EOVERFLOW;
            return NULL;
        }
    }
    if (q == output_end) {
        errno = EOVERFLOW;
        return NULL;
    }
    *q = '\0';
    return q;
}
