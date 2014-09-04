/**
 * \file page_alloc.c
 *  License details are found in the file LICENSE.
 * \brief
 *  Manipulate strings.
 * \author Taku Shimosawa  <shimosawa@is.s.u-tokyo.ac.jp> \par
 *      Copyright (C) 2011 - 2012  Taku Shimosawa
 */
/*
 * HISTORY
 */

#include <kmalloc.h>
#include <string.h>

size_t strlen(const char *p)
{
	const char *head = p;

	while(*p){
		p++;
	}
	
	return p - head;
}

size_t strnlen(const char *p, size_t maxlen)
{
	const char *head = p;

	while(*p && maxlen > 0){
		p++;
		maxlen--;
	}
	
	return p - head;
}

char *strcpy(char *dest, const char *src)
{
	char *head = dest;

	while((*(dest++) = *(src++)));

	return head;
}

char *strncpy(char *dest, const char *src, size_t maxlen)
{
	char *head = dest;
	ssize_t len = maxlen;

	while((*(dest++) = *(src++)) && --len);
	if(len > 0){
		while(len--){
			*(dest++) = '\0';
		}
	}

	return head;
}

int strcmp(const char *s1, const char *s2)
{
	while(*s1 && *s1 == *s2){
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while(*s1 && *s1 == *s2 && n > 1){
		s1++;
		s2++;
		n--;
	}
	return *s1 - *s2;
}

char *strchr(const char *s, int n) {
	char *p = (char *)s; 
	while (1) {
		if (*p == n)
		{
			return p;
		} else if (*p == '\0') {
			break;
		}
		++p;
	}
	return NULL;
}
		

char *strstr(const char *haystack, const char *needle)
{
	int len = strlen(needle);

	while(*haystack){
		if(!strncmp(haystack, needle, len)){
			return (char *)haystack;
		}
		haystack++;
	}
	return NULL;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const char *p1 = src;
	char *p2 = dest;

	while(n > 0){
		*p2 = *p1;
		p1++;
		p2++;
		n--;
	}

	return dest;
}

void *memcpy_long(void *dest, const void *src, size_t n)
{
	const unsigned long *p1 = src;
	unsigned long *p2 = dest;

	n /= sizeof(unsigned long);
	while (n > 0) {
		*(p2++) = *(p1++);
		n--;
	}

	return dest;
}

void *memset(void *s, int c, size_t n)
{
	char *s_aligned = (void *)(((unsigned long)s + 7) & ~7);
	char *e_aligned = (void *)(((unsigned long)s + n) & ~7);
	char *e = ((char *)s + n);
	char *p;
	unsigned long *l;
#define C ((unsigned long)(c & 0xff))
	unsigned long pat = C | C << 8 | C << 16 | C << 24 | C << 32 |
		C << 40 | C << 48 | C << 56;
#undef C

	if(s_aligned < e_aligned){
		p = s;
		while(p < s_aligned){
			*(p++) = (char)c;
		}
		l = (unsigned long *)s_aligned;
		while((char *)l < e_aligned){
			*(l++) = pat;
		}
		p = e_aligned;
		while(p < e){
			*(p++) = (char)c;
		}
	}else{
		p = s;
		while(p < e){
			*(p++) = (char)c;
		}
	}

	return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const char *p1 = s1;
	const char *p2 = s2;

	while(*p1 == *p2 && n > 1){
		p1++;
		p2++;
		n--;
	}
	return *p1 - *p2;
}

/* 
 * Flatten out a (char **) string array into the following format:
 * [nr_strings][char *offset of string_0]...[char *offset of string_n-1][NULL][string0]...[stringn_1]
 * if nr_strings == -1, we assume the last item is NULL 
 *
 * NOTE: copy this string somewhere, add the address of the string to each offset
 * and we get back a valid argv or envp array.
 *
 * returns the total length of the flat string and updates flat to
 * point to the beginning.
 */
int flatten_strings(int nr_strings, char **strings, char **flat)
{
	int full_len, string_i;
	unsigned long flat_offset;
	char *_flat;

	/* How many strings do we have? */
	if (nr_strings == -1) {
		for (nr_strings = 0; strings[nr_strings]; ++nr_strings); 
	}

	/* Count full length */
	full_len = sizeof(int) + sizeof(char *); // Counter and terminating NULL
	for (string_i = 0; string_i < nr_strings; ++string_i) {
		// Pointer + actual value
		full_len += sizeof(char *) + strlen(strings[string_i]) + 1; 
	}

	_flat = (char *)kmalloc(full_len, IHK_MC_AP_NOWAIT);
	if (!_flat) {
		return 0;
	}

	memset(_flat, 0, full_len);

	/* Number of strings */
	*((int*)_flat) = nr_strings;
	
	// Actual offset
	flat_offset = sizeof(int) + sizeof(char *) * (nr_strings + 1); 

	for (string_i = 0; string_i < nr_strings; ++string_i) {
		
		/* Fabricate the string */
		*((char **)(_flat + sizeof(int) + string_i * sizeof(char *))) = (void *)flat_offset;
		memcpy(_flat + flat_offset, strings[string_i], strlen(strings[string_i]) + 1);
		flat_offset += strlen(strings[string_i]) + 1;

	}

	*flat = _flat;
	return full_len;
}

