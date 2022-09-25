int __attribute__((naked)) lock_lock(volatile int *lock)
{
	asm volatile   ("mov	r1, r0\n"	\
			"\tmov	r3, #1\n"	\
			"try:\t\n"		\
			"\tldrex	r0, [r1]\n"	\
			"\tcmp	r0, #0\n"	\
			"\tstrexeq r0, r3, [r1]\n" \
			"\tcmpeq	r0,#0\n"	\
			"\tbne	try\n"	\
			"\tbx	lr\n"	\
		       );
}
