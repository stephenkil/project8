#!/usr/bin/python3

import sys
from collections import defaultdict, namedtuple

# Exception for when the IMM or REG functions find an inappropriate operand
class OperandException(Exception):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

REG_NAMES = {
        'ip': 'r0',
        'rp': 'r29',
        'fp': 'r30',
        'sp': 'r31',
        }

# convert a literal operand as it appears in the source file to
# how it should appear in the intermediate representation.
# If it's a number, then keep it as it is;
# otherwise, if it's a valid identifier, assume it's a label (including register names)
def IMM(operand, ofs, relocs):
    try:
        return [int(operand)]
    except ValueError:
        pass
    if not operand.isidentifier():
        raise OperandException(f"expected literal or label, found '{operand}'")
    relocs[operand].append(ofs)
    return [0]

# convert a register operand as it appears in the source file
# to how it should appear in the intermediate representation.
# Specifically, it better begin with R and then have a number from 0 to 32.
# If not, raise an appropriate exception
def REG(operand, ofs, relocs):
    try:
        operand = REG_NAMES[operand.casefold()]
    except KeyError:
        pass
    if operand[0].casefold() != 'r' or not operand[1:].isdigit():
        raise OperandException(f"expected register, found '{operand}'")
    elif int(operand[1:]) not in range(32):
        raise OperandException(f"invalid register '{operand}'")
    else:
        return [operand[1:]]

def DATA(operand, ofs, relocs):
    try:
        data_bytes = int(operand)
    except ValueError:
        data_bytes = -1
    if data_bytes < 0:
        raise OperandException(f"expected nonnegative integer, found '{operand}'")
    return [0] * data_bytes


# map from instruction mnemonics to lists describing the format of that
# instruction (numeric code + functions to convert/check the operands)
INSNS = {
        'movi': [[1], IMM, REG],
        'mov': [[2], REG, REG],
        'add': [[3], REG, REG],
        'sub': [[4], REG, REG],
        'mul': [[5], REG, REG],
        'idiv': [[6], REG, REG],
        'jmp': [[7], REG],
        'jnz': [[8], REG, REG],
        'out': [[9], REG],
        'halt': [[10]],
        'ld': [[11], REG, REG],
        'st': [[12], REG, REG],
        'jal': [[13], REG],
        'ret': [[14]],
        'push': [[15], REG],
        'pop': [[16], REG],
        'ldlo': [[17], IMM, REG],
        'stlo': [[18], IMM, REG],
        '.data': [[], DATA],
        }


# check to make sure there's a filename
if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} filename.asm")
    sys.exit(1)

# dump the file into a list (allowing us to refer back to lines for error reporting)
filename = sys.argv[1]
try:
    with open(filename) as asm_file:
        lines = list(asm_file)
except IOError:
    print(f"{sys.argv[0]}: error: unable to find or open file '{filename}'")
    sys.exit(1)
except UnicodeDecodeError:
    print(f"{sys.argv[0]}: error: file '{filename}' does not appear to be a text file")
    sys.exit(1)


# First pass: convert to machine language with unresolved labels

# Map from label names to (offset, line number) tuple
Label = namedtuple('Label', ['ofs', 'line'])
all_labels = {}

# Map from label names to a list of offsets which should contain the address
# of that label
relocs = defaultdict(list)

# Compiled machine language
compiled = []

# Don't quit first pass immediately on error; instead, remember whether there
# was an error and exit before second pass
error = False

def warn(lnr, msg, line=None):
    global filename, lines
    print(f"{filename}: warning: {msg}:")
    print(f" {lnr:3}: " + lines[lnr - 1].rstrip('\r\n'))

def err(lnr, msg, line=None):
    global filename, lines, error
    print(f"{filename}: error: {msg}:")
    print(f" {lnr:3}: " + lines[lnr - 1].rstrip('\r\n'))
    error = True

for lnr, orig_line in enumerate(lines, 1):
    orig_line = orig_line.rstrip('\r\n')

    # Split off anything after // as a comment
    line, _, comment = map(str.strip, orig_line.partition('//'))

    # Process any label(s) found at the beginning of the line
    labels, _, line = map(str.strip, line.rpartition(':'))
    if labels:
        # Store offset and line number of label
        newlbl = Label(len(compiled), lnr)

        # Associate this label with all label names on the line
        for lblname in map(str.strip, labels.split(':')):
            # Ignore "labels" which are probably offsets from disassembler
            if lblname.isdigit():
                continue

            # Require that labels be valid identifiers
            if not lblname.isidentifier():
                err(lnr, f"invalid label name '{lblname}'")
                continue

            # Ensure that each label is defined in only one place
            label = all_labels.get(lblname)
            if label:
                err(lnr, f"label '{lblname}' previously defined at {filename}:{label.line}")
                continue

            # Store the label
            all_labels[lblname] = newlbl

    try:
        op, *operands = line.split()
    except ValueError:
        # Nothing here but whitespace
        continue

    try:
        opcode, *fmts = INSNS[op.casefold()]
    except KeyError:
        err(lnr, f"unknown instruction '{op}'")
        continue

    if len(operands) != len(fmts):
        err(lnr, f"wrong number of operands to '{op}' (expected {len(fmts)}, got {len(operands)})")
        continue

    compiled += opcode
    for opnum, fmt_fn in enumerate(fmts):
        try:
            compiled += fmt_fn(operands[opnum], len(compiled), relocs)
        except OperandException as e:
            err(lnr, f"operand {opnum + 1}: {e.msg}")
            break

for labelname, label in all_labels.items():
    if labelname not in relocs:
        warn(label.line, f"unused label '{labelname}'")

if error:
    exit(1)

# Resolve labels
for lblname, offsets in relocs.items():
    try:
        lblofs = all_labels[lblname].ofs
    except KeyError:
        err(lnr, f"undefined label '{lblname}'")
        continue
    for ofs in offsets:
        compiled[ofs] = lblofs

if error:
    exit(1)

# if the source file ends with .asm, then replace it with .vml, otherwise
# just add .vml
if filename.endswith('.asm'):
    filename = filename[:-4]
out_filename = filename + '.vml'

# write the program
with open(out_filename, 'w') as f:
    print(len(compiled), file=f)
    print("\n".join(map(str, compiled)), file=f)
