/* tf_str.h — minimal freestanding string/mem functions for the pi32v2 build.
 * On host: just <string.h>. On the chip: tiny inline versions (the JieLi SDK also
 * provides these; this keeps our modules buildable standalone).
 */
#ifndef TF_STR_H
#define TF_STR_H
#if defined(__pi32v2__) || defined(TF_EMBED)
#include <stddef.h>
static inline void *tf_memcpy(void *d, const void *s, size_t n) {
    unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d;
}
static inline void *tf_memset(void *d, int c, size_t n) {
    unsigned char *a = d; while (n--) *a++ = (unsigned char)c; return d;
}
static inline int tf_memcmp(const void *x, const void *y, size_t n) {
    const unsigned char *a = x, *b = y; for (; n--; a++, b++) if (*a != *b) return *a - *b; return 0;
}
static inline int tf_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b;
}
static inline char *tf_strncpy(char *d, const char *s, size_t n) {
    size_t i = 0; for (; i < n && s[i]; i++) d[i] = s[i]; for (; i < n; i++) d[i] = 0; return d;
}
static inline size_t tf_strnlen(const char *s, size_t m) { size_t i = 0; while (i < m && s[i]) i++; return i; }
#define memcpy  tf_memcpy
#define memset  tf_memset
#define memcmp  tf_memcmp
#define strcmp  tf_strcmp
#define strncpy tf_strncpy
#define strnlen tf_strnlen
#else
#include <string.h>
#endif
#endif /* TF_STR_H */
