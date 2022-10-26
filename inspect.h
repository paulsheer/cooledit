/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef NO_INSPECT
#define NO_INSPECT
#endif

#if defined(NO_INSPECT) || !defined(__GNUC__) || !defined(__x86_64__)

#define E_

void init_inspect (void);
void inspect_clean_exit (void);
void housekeeping_inspect (void);

#else


#define INSPECT_LEN             72
#define INSPECT_STACK_LEN       2048


void init_inspect (void);
void inspect_clean_exit (void);
void housekeeping_inspect (void);

struct inspecti_st__ {
    char line[INSPECT_LEN];
};

struct inspect_st__ {
    char timestamp[16];
    unsigned int dummy;
    unsigned int current;
    struct inspecti_st__ d[INSPECT_STACK_LEN];
};

extern struct inspect_st__ *inspect_data__;

/* 
 * Required:
 *  echo 0 > /proc/sys/kernel/randomize_va_space
 *  and compiler option -no-pie
 * 
 * Get timestamp with:
 *  cat /root/.cedit/inspect/inspect12345 | tr -d '\0' | grep -a '^T' ; 
 * 
 * Get stacktrace with:
 *  cat /root/.cedit/inspect/inspect12345 | tr -d '\0' | grep -a -v '^[T#]' | grep -a -v '^$' > tmp_ && cat tmp_ | sed -e '1,/=/D' && cat tmp_ | sed -e '1,/=/!D' ;
 * 
 */
static inline void inspect___ (const char *file, const char *function, void *address)
{
    const char *f = file;
    const char *u = function;
    char *l_, *le;
    int i, n;
    unsigned int c;
    unsigned hi, lo;
    unsigned long long t;
    unsigned long a;

    if (!inspect_data__)
        return;

    l_ = inspect_data__->d[inspect_data__->current].line;
    le = l_ + INSPECT_LEN;

    asm volatile ("rdtsc":"=a" (lo), "=d" (hi));
    t = ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
    t /= 1000;

    c = (inspect_data__->current + INSPECT_STACK_LEN - 1) % INSPECT_STACK_LEN;
    inspect_data__->d[c].line[1] = ' ';

    l_[0] = '\n';
    l_[1] = '=';
    for (i = 13; i >= 2; i--) {
        l_[i] = '0' + (t % 10);
        t /= 10;
    }
    l_[14] = ':';

    a = (unsigned long) address;
    n = 16 - __builtin_clzl (a) / 4;
    for (i = 15 + n; i >= 15; i--) {
        l_[i] = "0123456789abcdef"[a & 0xf];
        a >>= 4;
    }
    l_ += 16 + n;
    *l_++ = ':';
    while (*f && l_ < le)
        (*l_++ = *f++);
    *l_++ = ':';
    while (*u && l_ < le)
        (*l_++ = *u++);
    *l_++ = '\n';
    *l_ = '#';

    inspect_data__->current = (inspect_data__->current + 1) % INSPECT_STACK_LEN;
}

#define E_      { inspect___(__FILE__, __FUNCTION__, __builtin_extract_return_addr(__builtin_return_address(0))); }

#endif

