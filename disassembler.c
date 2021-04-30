#include <stdio.h>
#include <stdlib.h>

#include "asm.h"

#define MEMSIZE 500

int
main(int argc, char **argv)
{
	int instr_words;
	int memory[MEMSIZE];
	int ip;

	if (argc < 2) {
		fprintf(stderr,
				"Missing filename.\n"
				"Usage: %s program.vml\n", argv[0]);
		return 1;
	}

	instr_words = read_vmlfile(argv[1], memory, MEMSIZE);

	for (ip = 0; ip < instr_words; /* */) {
		int newip = disassemble(memory, ip);
		if (newip < 0)
			return 1;
		ip = newip;
	}

	return 0;
}
