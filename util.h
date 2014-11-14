#ifndef UTIL_H
#define UTIL_H

#include "andromon.h"

#define DEBUG 1

char* utf8(const char *str)
{
	char *utf8;
	utf8 = kmalloc(1+(2*strlen(str)), GFP_KERNEL);

	if (utf8) {
		char *c = utf8;
		for (; *str; ++str) {
			if (*str & 0x80) {
				*c++ = *str;
			} else {
				*c++ = (char) (0xc0 | (unsigned) *str >> 6);
				*c++ = (char) (0x80 | (*str & 0x3f));
			}
		}
		*c++ = '\0';
	}
	return utf8;
}

void Debug_Print(char* TAG, char *msg){
#if DEBUG
	printk(KERN_INFO "%s: %s\n", TAG, msg);
#endif
}
#endif
