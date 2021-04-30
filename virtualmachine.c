#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "asm.h"

/* Architecture definitions */
#define MEMSIZE 500
#define REGS 32
#define IP 0
#define RP 29
#define FP 30
#define SP 31

/* Other constants */
#define MAXLINE 255

struct machine {
	char **argv;
	int argc;
	int reg[REGS];
	int mem[MEMSIZE];
};

struct debugger {
	int enable;
	int pause;
	int breakpoint;
};

/*
 * Prints debugger help message
 */
void help(void)
{
	fprintf(stderr,
"Debugger commands (may be abbreviated):\n"
"  help      - Prints this message\n"
"  quit      - Exits virtual machine completely\n"
"  list [N]  - Disassembles the next N instructions (default 5)\n"
"  print R   - Prints the value of the register named R\n"
"              (also accepts mnemonics IP, RP, FP, SP)\n"
"  x A       - Prints the value in memory at address A\n"
"              (also accepts register names if register holds an address)\n"
"  frame [N] - Displays the top N values on the stack\n"
"              (default from SP to FP)\n"
//"  step [N]  - Executes N instructions (default 1)\n"
"  step      - Executes the next instruction\n"
"  continue  - Runs the program from current state without interruption\n"
"  restart   - Resets the program to initial state\n"
"  break [A] - Creates a `breakpoint' to pause execution whenever IP == A\n"
"              (default is current IP)\n"
"  delete    - Deletes the breakpoint\n"
"\n"
"Commands which accept an address will also accept a register name, in\n"
"which case the value in that register will be used as the address.\n"
"\n"
"Commands which accept a register name also recognize mnemonics for\n"
"special-purpose registers (ip, rp, fp, sp).\n");
}

int in_mem(int addr)
{
	return addr >= 0 && addr < MEMSIZE;
}

int reset_machine(struct machine *vm)
{
	int code_size = read_vmlfile(vm->argv[0], vm->mem, MEMSIZE);
	if (code_size < 0)
		return 1;

	// Add an invalid opcode to the code segment to separate from stack
	vm->mem[code_size++] = -1;

	vm->reg[IP] = 0;
	vm->reg[SP] = code_size;
	// Push arguments to main
	for (int i = 1; i < vm->argc; i++)
		vm->mem[vm->reg[SP]++] = atoi(vm->argv[i]);
	vm->reg[FP] = vm->reg[SP];
	return 0;
}

int get_reg_arg(char *arg)
{
	int regnum;
	if (!strcasecmp(arg, "ip"))
		return IP;
	if (!strcasecmp(arg, "rp"))
		return RP;
	if (!strcasecmp(arg, "fp"))
		return FP;
	if (!strcasecmp(arg, "sp"))
		return SP;
	if (arg[0] != 'r' && arg[0] != 'R')
		return -1;

	regnum = atoi(arg + 1);
	if (regnum < 0 || regnum >= REGS)
		return -1;
	return regnum;
}

int get_addr_arg(char *arg, int *regs)
{
	int regnum, addr;
	if (arg[0] == '\0') {
		fprintf(stderr, "address argument is required\n");
		return -1;
	}

	// Try register name first, then address
	regnum = get_reg_arg(arg);
	if (regnum >= 0) {
		addr = regs[regnum];
	} else if (isdigit(arg[0])) {
		addr = atoi(arg);
	} else {
		fprintf(stderr, "bad address: `%s'\n", arg);
		return -1;
	}

	// Check that address is within memory
	if (!in_mem(addr)) {
		fprintf(stderr, "address %d out of range\n", addr);
		return -1;
	}
	return addr;
}

/*
 * from https://stackoverflow.com/a/4770992/656767
 */
int is_prefix(char *pre, char *str)
{
	return !strncasecmp(pre, str, strlen(pre));
}

/*
 * Executes one instruction.
 *
 * Returns 0 for success, -1 to halt the machine, and 1 if there is an error.
 */
#define FETCH(name) do { \
		if (!in_mem(ip)) { \
			fprintf(stderr, "instruction fetch at %d out of range\n", ip); \
			return 1; \
		} \
		(name) = memory[ip++]; \
	} while (0)
#define IMM(name) do { FETCH((name)); } while (0)
#define REG(name) do { \
		FETCH((name)); \
		if ((name) < 0 || (name) >= REGS) { \
			fprintf(stderr, "register operand %d out of range\n", (name)); \
			return 1; \
		} \
	} while (0)
#define MEM(addr) ({ \
		if (!in_mem((addr))) { \
			fprintf(stderr, "memory access %d out of range\n", (addr)); \
			return 1; \
		} \
		memory[addr]; \
	})
int step(struct machine *vm)
{
	int *registers = vm->reg;
	int *memory = vm->mem;
	int ip = registers[IP];
	int opcode, rd, rs, ra, imm, addr;

	FETCH(opcode);
	switch (opcode)
	{
		case 1:  /* MOVI */
			IMM(imm); REG(rd);
			registers[rd] = imm;
			break;

		case 2:  /* MOV  rs rd */
			REG(rs); REG(rd);
			registers[rd] = registers[rs];
			break;

		case 3:  /* ADD  rs rd */
			REG(rs); REG(rd);
			registers[rd] += registers[rs];
			break;

		case 4:  /* SUB  rs rd */
			REG(rs); REG(rd);
			registers[rd] -= registers[rs];
			break;

		case 5:  /* MUL  rs rd */
			REG(rs); REG(rd);
			registers[rd] *= registers[rs];
			break;

		case 6:  /* IDIV rs rd */
			REG(rs); REG(rd);
			registers[rd] /= registers[rs];
			break;

		case 7:  /* JMP  ra */
			REG(ra);
			ip = registers[ra];
			break;

		case 8:  /* JNZ  rs ra */
			REG(rs); REG(ra);
			if (registers[rs])
				ip = registers[ra];
			break;

		case 9:  /* OUT  rs */
			REG(rs);
			printf("%d\n", registers[rs]);
			break;

		case 10: /* HALT */
			return -1;

		case 11: /* LD   ra rd */
			REG(ra); REG(rd);
			if (!in_mem(registers[ra])) {
				fprintf(stderr, "LD address %d out of range\n",
						registers[ra]);
				return 1;
			}
			registers[rd] = memory[registers[ra]];
			break;

		case 12: /* ST   ra rs */
			REG(ra); REG(rs);
			if (!in_mem(registers[ra])) {
				fprintf(stderr, "ST address %d out of range\n",
						registers[ra]);
				return 1;
			}
			memory[registers[ra]] = registers[rs];
			break;

		case 13: /* JAL  ra */
			REG(ra);
			registers[RP] = ip;
			ip = registers[ra];
			break;

		case 14: /* RET */
			ip = registers[RP];
			break;

		case 15: /* PUSH rs */
			REG(rs);
			if (!in_mem(registers[SP])) {
				fprintf(stderr, "SP (%d) out of range for PUSH\n",
						registers[SP]);
				return 1;
			}
			memory[registers[SP]] = registers[rs];
			registers[SP]++;
			break;

		case 16: /* POP  rd */
			REG(rd);
			registers[SP]--;
			if (!in_mem(registers[SP])) {
				fprintf(stderr, "SP (%d) out of range for POP\n",
						registers[SP]);
				return 1;
			}
			registers[rd] = memory[registers[SP]];
			break;

		case 17: /* LDLO imm rd */
			IMM(imm); REG(rd);
			addr = registers[FP] + imm;
			if (!in_mem(addr)) {
				fprintf(stderr, "LDLO (%d = FP + %d) out of range\n",
						addr, imm);
				return 1;
			}
			registers[rd] = memory[addr];
			break;

		case 18: /* STLO imm rs */
			IMM(imm); REG(rs);
			addr = registers[FP] + imm;
			if (!in_mem(addr)) {
				fprintf(stderr, "STLO (%d = FP + %d) out of range\n",
						addr, imm);
				return 1;
			}
			memory[addr] = registers[rs];
			break;

		default:
			fprintf(stderr, "invalid opcode: %d\n", memory[registers[IP]]);
			return 1;
	}
	// Update IP only if no error
	registers[IP] = ip;

	return 0;
}
#undef REG
#undef IMM
#undef FETCH

int read_command(char *cmd, char *arg, int ip) {
	char line[MAXLINE + 1];
	int argc = 0;
	do {
		// Display prompt and read a line
		fprintf(stderr, "\e[1;33mdbg[%03d]>\e[0m ", ip);
		if (!fgets(line, MAXLINE, stdin)) {
			if (ferror(stdin))
				fprintf(stderr, "Unexpected error when reading command\n");
			return -1;
		}
		// Clear buffers and parse command
		cmd[0] = arg[0] = '\0';
		argc = sscanf(line, " %s %s ", cmd, arg);
	} while (argc < 1);

	return argc;
}

void print_stack(struct machine *vm, int num)
{
	int sp = vm->reg[SP], fp = vm->reg[FP];
	int addr;

	for (addr = sp; addr >= sp - num; addr--) {
		if (addr != sp) {
			if (in_mem(addr))
				fprintf(stderr, " %d", vm->mem[addr]);
			else
				fprintf(stderr, " XX");
		} else {
			fprintf(stderr, "[SP]");
		}
		if (addr == fp)
			fprintf(stderr, "[FP]");
	}
	fprintf(stderr, "\n");

	if (!in_mem(sp - 1) || !in_mem(sp - num))
		fprintf(stderr, "Note: some addresses (XX) were out of range\n");
}

int main(int argc, char **argv)
{
	struct machine vm;
	struct debugger dbg = {
		.enable = 0,
		.pause = 0,
		.breakpoint = -1,
	};

	// Parse command-line arguments
	int filearg = 1;
	if (argc > 1 && !strcmp(argv[1], "-d")) {
		dbg.enable = 1;
		filearg++;
	}

	if (filearg >= argc) {
		fprintf(stderr,
			"Missing filename.\n"
			"Usage: %s [-d] program.vml [args...]\n", argv[0]);
		return 1;
	}

	vm.argv = argv + filearg;
	vm.argc = argc - filearg;

	if (reset_machine(&vm)) {
		return 1;
	}

	// Start paused if debugging
	dbg.pause = dbg.enable;

	for(;;) {
		char cmd[256], arg[256];
		int args;

		if (dbg.enable) {
			// Handle breakpoint
			if (!dbg.pause && vm.reg[IP] == dbg.breakpoint) {
				fprintf(stderr, "Hit breakpoint at %d\n", dbg.breakpoint);
				dbg.pause = 1;
			}

			if (dbg.pause) {
				// Display a prompt, read a command and parse it
				args = read_command(cmd, arg, vm.reg[IP]);
				if (args < 0) {
					fprintf(stderr, "\nEnd of input, continuing program\n");
					dbg.enable = 0;
				}
			}
		}

		if (dbg.enable && dbg.pause && is_prefix(cmd, "continue")) {
			// Continue (make sure one step happens to avoid
			// immediately re-pausing if at a breakpoint)
			dbg.pause = 0;
		}

		// Run one step of the machine (default if not in debug mode or
		// if not paused)
		if (!dbg.enable || !dbg.pause || is_prefix(cmd, "step")) {
			switch (step(&vm)) {
				case 0:
					// Normal step
					break;
				case 1:
					// Error
					if (!dbg.enable)
						return 1;
					dbg.pause = 1;
					break;
				case -1:
					// Halt instruction
					if (!dbg.enable)
						return 0;
					fprintf(stderr, "[Program exited normally]\n");
					dbg.pause = 1;
					break;
			}
		} else if (is_prefix(cmd, "continue")) {
			// Continue (already handled in a previous if)
		} else if (is_prefix(cmd, "help")) {
			// Help
			help();
		} else if (is_prefix(cmd, "quit")) {
			// Quit
			break;
		} else if (is_prefix(cmd, "list")) {
			// List disassembly
			int num = 5, addr = vm.reg[IP];
			if (args >= 2) {
				num = atoi(arg);
			}
			int i;
			for (i = 0; i < num && in_mem(addr); i++)
				addr = disassemble(vm.mem, addr);
		} else if (is_prefix(cmd, "print")) {
			// Print register
			if (args < 2) {
				fprintf(stderr, "print command requires an argument\n");
			} else {
				int regnum = get_reg_arg(arg);
				if (regnum < 0) {
					fprintf(stderr, "bad register name: `%s'\n", arg);
				} else {
					fprintf(stderr, "%s: %d\n", arg, vm.reg[regnum]);
				}
			}
		} else if (!strcmp(cmd, "x")) {
			// Examine memory
			if (args < 2) {
				fprintf(stderr, "x command requires an argument\n");
			} else {
				int addr = get_addr_arg(arg, vm.reg);
				if (addr >= 0)
					fprintf(stderr, "%03d: %d\n", addr, vm.mem[addr]);
			}
		} else if (is_prefix(cmd, "frame")) {
			// Show stack values
			int num;
			if (args >= 2) {
				num = atoi(arg);
			} else {
				if (vm.reg[SP] < vm.reg[FP]) {
					fprintf(stderr, "Bad frame: SP (%d) is below FP (%d)\n",
							vm.reg[SP], vm.reg[FP]);
					continue;
				} else {
					num = vm.reg[SP] - vm.reg[FP] + 1;
				}
			}
			print_stack(&vm, num);
		} else if (is_prefix(cmd, "restart")) {
			// Restart same program
			if (reset_machine(&vm)) {
				fprintf(stderr, "Machine reset failed, exiting\n");
				return 1;
			}
		} else if (is_prefix(cmd, "break")) {
			// Set breakpoint
			int addr = vm.reg[IP];
			if (args >= 2)
				addr = get_addr_arg(arg, vm.reg);
			if (addr >= 0) {
				dbg.breakpoint = addr;
				fprintf(stderr, "Breakpoint set at address %d\n", addr);
			}
		} else if (is_prefix(cmd, "delete")) {
			// Delete breakpoint
			if (dbg.breakpoint < 0) {
				fprintf(stderr, "No breakpoint set\n");
			} else {
				fprintf(stderr, "Breakpoint deleted (was %d)\n", dbg.breakpoint);
				dbg.breakpoint = -1;
			}
		}
	}
	return 0;
}
