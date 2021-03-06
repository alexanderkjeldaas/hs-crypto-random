/*
 * Copyright (C) Thomas DuBuisson
 * Copyright (C) 2013 Vincent Hanquez <tab@snarc.org>
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int crypto_random_cpu_has_rdrand()
{
	uint32_t ax,bx,cx,dx,func=1;
	__asm__ volatile ("cpuid": "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
	return (cx & 0x40000000);
}

/* sadly many people are still using an old binutils,
 * leading to report that instruction is not recognized.
 */
#if 0
/* Returns 1 on success */
static inline int crypto_random_rdrand64_step(uint64_t *buffer)
{
	unsigned char err;
	asm volatile ("rdrand %0; setc %1" : "=r" (*buffer), "=qm" (err));
	return (int) err;
}
#endif

/* inline encoding of 'rdrand %rax' to cover old binutils
 * - no inputs
 * - 'cc' to the clobber list as we modify condition code.
 * - output of rdrand in rax and have a 8 bit error condition
 */
#define inline_rdrand_rax(val, err) \
	asm(".byte 0x48,0x0f,0xc7,0xf0; setc %1" \
	   : "=a" (val), "=q" (err) \
	   : \
	   : "cc")

/* Returns the number of bytes succesfully generated */
int crypto_random_get_rand_bytes(uint8_t *buffer, size_t len)
{
	uint64_t tmp;
	int aligned = (unsigned long) buffer % 8;
	int orig_len = len;
	int to_alignment = 8 - aligned;
	uint8_t ok;

	if (aligned != 0) {
		inline_rdrand_rax(tmp, ok);
		if (!ok)
			return 0;
		memcpy(buffer, (uint8_t *) &tmp, to_alignment);
		buffer += to_alignment;
		len -= to_alignment;
	}

	for (; len >= 8; buffer += 8, len -= 8) {
		inline_rdrand_rax(tmp, ok);
		if (!ok)
			return (orig_len - len);
		*((uint64_t *) buffer) = tmp;
	}

	if (len > 0) {
		inline_rdrand_rax(tmp, ok);
		if (!ok)
			return (orig_len - len);
		memcpy(buffer, (uint8_t *) &tmp, len);
	}
	return orig_len;
}
