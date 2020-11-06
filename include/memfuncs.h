#ifndef MEMFUNCS_H
#define MEMFUNCS_H

#include <stdint.h>

void info_msg(char *msg);

#ifdef BUILD_IMAGE
int memcmp(void *str1, void *str2, long n) {
    uint8_t *x = (uint8_t *)str1;
    uint8_t *y = (uint8_t *)str2;
    for (long i = 0; i < n; i++) {
        if (x[i] == y[i]) {
            continue;
        }
        
        return (x[i] > y[i]) ? 1 : -1;
    }

    return 0;
}

void *memcpy(void *dest, const void *src, long n) {
#ifndef USE_ASM
    for (long i = 0; i < n; i++) {
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    }

    return dest;
#else
    asm(
        "cld\n"
        "movl %0, %%esi\n"
        "movl %1, %%edi\n"
        "movl %2, %%ecx\n"
        "rep movsb\n"
        :
        : "g" (src), "g" (dest), "g" (n)
        : "ecx", "edi", "esi"
    );

    return dest;
#endif
}

void *memmove(void *dest, const void *src, long n) {
    memcpy(dest, src, n);
}

void *memset(void *s, int c, long n) {
#ifndef USE_ASM
    for (long i = 0; i < n; i++) {
        ((uint8_t *)s)[i] = c;
    }

    return s;
#else
    asm(
        "cld\n"
        "movl %0, %%edi\n"
        "movb %1, %%al\n"
        "movl %2, %%ecx\n"
        "rep stosb\n"
        :
        : "g" (s), "g" ((unsigned char) c), "g" (n)
        : "ecx", "edi", "al"
    );

    return s;
#endif
}

int strlen(char *s) {
#ifndef USE_ASM
    int i = 0;
    for (i = 0; s[i]; i++);
    return i;
#else
    char *x = 0;
    asm(
        "cld\n"
        "movl %1, %%edi\n"
        "xorb %%al, %%al\n"
        "xorl %%ecx, %%ecx\n"
        "notl %%ecx\n"
        "repne scasb\n"
        "movl %%edi, %0\n"
        : "=g" (x)
        : "g" (s)
        : "ecx", "edi", "al"
    );

    return x - s;
#endif
}
#else
#include <string.h>
#include <stdlib.h>
#endif

char *kstrcat(char *a, char *b, char *c) {
    char *tmp = c;

    while (*a) {
        *c++ = *a++;
    }

    while (*b) {
        *c++ = *b++;
    }

    *c = 0;

    return tmp;
}

/*
 * write uint8 as hex to str
 */
char *kstru8hex(char *str, uint8_t x) {
    char tmp[3] = {0};
    char *table = "0123456789abcdef";
    tmp[0] = table[x >> 4];
    tmp[1] = table[x & 0xf];
    tmp[2] = 0;
    kstrcat(str, tmp, str);
    return str;
}

/*
 * same as kstru8hex but uint16
 */
char *kstru16hex(char *str, uint16_t x) {
    kstru8hex(str, x >> 8);
    kstru8hex(str, x & 0xff);
    return str;
}

/*
 * same as kstru8hex but uint32
 */
char *kstru32hex(char *str, uint32_t x) {
    kstru16hex(str, x >> 16);
    kstru16hex(str, x & 0xffff);
    return str;
}

void print_u32h(uint32_t x) {
    char str[32] = "value: ";
    kstru32hex(str, x);
    info_msg(str);
}

uint32_t divide(uint32_t a, uint32_t b) {
    uint32_t res = 0;
    asm(
        "movl %1, %%eax\n"
        "xorl %%edx, %%edx\n"
        "divl %2\n"
        "movl %%eax, %0\n"
        : "=g" (res)
        : "r" (a), "r" (b)
        : "eax", "edx"
    );
    return res;
}

uint32_t modulo(uint32_t a, uint32_t b) {
    uint32_t res = 0;
    asm(
        "movl %1, %%eax\n"
        "xorl %%edx, %%edx\n"
        "divl %2\n"
        "movl %%edx, %0\n"
        : "=g" (res)
        : "r" (a), "r" (b)
        : "eax", "edx"
    );
    return res;
}

char *kstrdec(char *str, int32_t x) {
    if (x == 0) {
        kstrcat(str, "0", str);
        return str;
    }

    if (x < 0) {
        kstrcat(str, "-", str);
        x = -x;
    }

    char tmp[16] = {0};
    for (int i = 0; i < 12; i++) {
        tmp[i] = '0' + modulo(x, 10);
        x = divide(x, 10);
    }
    char tmp2[16] = {0};
    for (int i = 0; i < 12; i++) {
        tmp2[11 - i] = tmp[i];
    }

    char *ptr = tmp2;
    while (*ptr == '0') {
        ptr++;
    }

    kstrcat(str, ptr, str);

    return str;
}
#endif