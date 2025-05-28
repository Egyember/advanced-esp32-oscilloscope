#include <helpertypes.h>
#include <pthread.h>

#include <stdio.h>
int helper::hexdump(unsigned char *src, size_t len, unsigned int with){
	if (!src) {
		return -1;
	}
	unsigned int l = 0;
	for (size_t i = 0; i < len; i++) {
		printf("%02x", src[i]);
		if (++l == with) {
			printf("\n");
			l = 0;
		}
	}
	printf("\n");
	return 0;
};

