.PHONY: all clean

BINS = disassembler virtualmachine
OBJS = asm.o $(addsuffix .o,$(BINS))
CFLAGS = -g -Wall -Wextra -std=gnu99
LDFLAGS = -g

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

asm.o: asm.h
disassembler.o: asm.h
disassembler: asm.o
virtualmachine.o: asm.h
virtualmachine: asm.o
