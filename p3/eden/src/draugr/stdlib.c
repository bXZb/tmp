/**
* Copy memory from one location to another.
*
* @param dest A pointer to the destination buffer
* @param src A pointer to the source buffer
* @param size The size of the memory to copy
* @return A Boolean value to indicate success
*/
BOOL _memcpy(void* dest, void* src, size_t size) {
    if (dest == NULL || src == NULL) {
        return FALSE;
    }
    char* csrc = (char*)src;
    char* cdest = (char*)dest;
    for (size_t i = 0; i < size; i++) {
        cdest[i] = csrc[i];
    }
    return TRUE;
}

/**
* Move a specified byte into a memory location.
*
* @param ptr A pointer to the destination buffer
* @param byte The byte to copy into memory
* @param size The number of bytes to copy
*/
void* _memset(void* ptr, int byte, size_t size) {
    for (int i = 0; i < size; i++) {
        ((char*)ptr)[i] = byte;
    }
    return ptr;
}

/**
 * A function to compare two memory blocks.
 *
 * @param ptr1 A pointer to the destination buffer
 * @param ptr2 A pointer to the source buffer
 * @param size Number of bytes to compare
 * @return A negative value if ptr1 is less than ptr2, a positive value if ptr1 is greater
 *         than ptr2, or 0 if both memory blocks are equal
*/
int _memcmp(const void* ptr1, const void* ptr2, size_t size) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;

    for (size_t i = 0; i < size; ++i) {
        if (p1[i] < p2[i]) {
            return -1;
        }
        else if (p1[i] > p2[i]) {
            return 1;
        }
    }

    return 0;
}

/**
 * A function to compare two strings.
 *
 * @param s1 A pointer to the first string to compare
 * @param s2 A pointer to the second string to compare
 * @return Zero if strings match, +1 at first mismatch if character in s1 is greater, -1 at first mismatch if character in s2 is greater


A negative value if ptr1 is less than ptr2, a positive value if ptr1 is greater
 *         than ptr2, or 0 if both memory blocks are equal
*/
/* for ref: https://stackoverflow.com/questions/43672982/how-to-make-strcmp-function */
int _strcmp(const char *s1, const char *s2)
{
    for ( ; *s1 == *s2; s1++, s2++)
        if (*s1 == '\0') {
            return 0;
        }
    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
 }
