// mac_finder_sort.c
// Finder-like sorting for macOS, minimal integration with tree
// Compiled only on Darwin; provides findersort() used by --sort=finder

#include "tree.h"

#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#include <CoreFoundation/CoreFoundation.h>
#include <string.h>

extern bool reverse;

/*
 * Compare two _info entries by their names using Finder-like collation.
 * Flags chosen to match Finder-like behavior:
 *  - kCFCompareLocalized: use user locale collation
 *  - kCFCompareNumerically: numeric-aware sorting
 *  - kCFCompareDiacriticInsensitive: ignore diacritics (accents/marks)
 *  - kCFCompareWidthInsensitive: ignore width (fullwidth/halfwidth)
 */
int findersort(struct _info **a, struct _info **b)
{
    if (a == NULL || b == NULL || *a == NULL || *b == NULL) return 0;

    const char *n1 = (*a)->name ? (*a)->name : "";
    const char *n2 = (*b)->name ? (*b)->name : "";

    int result = 0;

    CFStringRef s1 = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, n1);
    CFStringRef s2 = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, n2);

    if (s1 && s2) {
        CFOptionFlags flags = kCFCompareLocalized
                            | kCFCompareNumerically
                            | kCFCompareDiacriticInsensitive
                            | kCFCompareWidthInsensitive;

        CFComparisonResult r = CFStringCompare(s1, s2, flags);
        if (r == kCFCompareLessThan) result = -1;
        else if (r == kCFCompareGreaterThan) result = 1;
        else result = 0;

        // Optional tie-breaker to keep ordering deterministic:
        if (result == 0) {
            // Fallback to byte-wise compare to stabilize sort
            result = strcmp(n1, n2);
            if (result < 0) result = -1;
            else if (result > 0) result = 1;
            else result = 0;
        }
    } else {
        // If CFString creation failed, fall back to strcmp
        int v = strcmp(n1, n2);
        if (v < 0) result = -1;
        else if (v > 0) result = 1;
        else result = 0;
    }

    if (s1) CFRelease(s1);
    if (s2) CFRelease(s2);

    return reverse ? -result : result;
}

#endif /* __APPLE__ */