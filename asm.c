#include <stdio.h>

#include "asm.h"

int read_vmlfile(char *fname, int *memory, int maxsize)
{
	int words, i;
	FILE *source = fopen(fname, "r");
	if (!source) {
		fprintf(stderr, "File not found: `%s'\n", fname);
		return -1;
	}
	if (fscanf(source, " %d ", &words) != 1) {
		fprintf(stderr, "No program size in first line of %s\n", fname);
		return -1;
	}
	if (words < 0) {
		fprintf(stderr, "Invalid program size %d in %s\n", words, fname);
		return -1;
	}
	if (words > maxsize) {
		fprintf(stderr, "Program size %d too large for memory\n", words);
		return -1;
	}

	for(i = 0; i < words; i++) {
		if (fscanf(source, " %d ", memory + i) != 1) {
			if (feof(source))
				fprintf(stderr, "File %s advertised %d words but had only %d\n",
						fname, words, i);
			else
				fprintf(stderr, "Bad data for word %d in file %s\n", i, fname);
			return -1;
		}
	}

	if (!feof(source)) {
		fprintf(stderr, "Bad VML file format (trailing data after %d words)\n",
				words);
		return -1;
	}

	fclose(source);
	return words;
}

int
disassemble(int *memory, int ip)
{
	int rd, rs, ra, imm;
	// Starting address of the instruction
	int addr = ip;
	switch (memory[ip++])
	{
		case 1:  /* MOVI imm rd */
			imm = memory[ip++];
			rd = memory[ip++];
			printf("%03d: MOVI %d r%d\n", addr, imm, rd);
			break;

		case 2:  /* MOV  rs rd */
			rs = memory[ip++];
			rd = memory[ip++];
			printf("%03d: MOV  r%d r%d\n", addr, rs, rd);
			break;

		case 3:  /* ADD  rs rd */
			rs = memory[ip++];
			rd = memory[ip++];
			printf("%03d: ADD  r%d r%d\n", addr, rs, rd);
			break;

		case 4:  /* SUB  rs rd */
			rs = memory[ip++];
			rd = memory[ip++];
			printf("%03d: SUB  r%d r%d\n", addr, rs, rd);
			break;

		case 5:  /* MUL  rs rd */
			rs = memory[ip++];
			rd = memory[ip++];
			printf("%03d: MUL  r%d r%d\n", addr, rs, rd);
			break;

		case 6:  /* IDIV rs rd */
			rs = memory[ip++];
			rd = memory[ip++];
			printf("%03d: IDIV r%d r%d\n", addr, rs, rd);
			break;

		case 7:  /* JMP  ra */
			ra = memory[ip++];
			printf("%03d: JMP  r%d\n", addr, ra);
			break;

		case 8:  /* JNZ  rs, ra */
			rs = memory[ip++];
			ra = memory[ip++];
			printf("%03d: JNZ  r%d r%d\n", addr, rs, ra);
			break;

		case 9:  /* OUT  rs */
			rs = memory[ip++];
			printf("%03d: OUT  r%d\n", addr, rs);
			break;

		case 10: /* HALT */
			printf("%03d: HALT\n", addr);
			break;

		case 11: /* LD   ra rd */
			ra = memory[ip++];
			rd = memory[ip++];
			printf("%03d: LD   r%d r%d\n", addr, ra, rd);
			break;

		case 12: /* ST   ra rs */
			ra = memory[ip++];
			rs = memory[ip++];
			printf("%03d: ST   r%d r%d\n", addr, ra, rs);
			break;

		case 13: /* JAL  ra */
			ra = memory[ip++];
			printf("%03d: JAL  r%d\n", addr, ra);
			break;

		case 14: /* RET */
			printf("%03d: RET\n", addr);
			break;

		case 15: /* PUSH rs */
			rs = memory[ip++];
			printf("%03d: PUSH r%d\n", addr, rs);
			break;

		case 16: /* POP  rd */
			rd = memory[ip++];
			printf("%03d: POP  r%d\n", addr, rd);
			break;

		case 17: /* LDLO imm rd */
			imm = memory[ip++];
			rd = memory[ip++];
			printf("%03d: LDLO %d r%d\n", addr, imm, rd);
			break;

		case 18: /* STLO imm rs */
			imm = memory[ip++];
			rs = memory[ip++];
			printf("%03d: STLO %d r%d\n", addr, imm, rs);
			break;

		default:
			fprintf(stderr, "%03d: invalid opcode: %d\n", addr, memory[addr]);
			return -1;
	}
	return ip;
}
