import sys
import json
from pprint import pprint
from dataclasses import dataclass
from textwrap import dedent

flag_alias = {
        'Z':  'ot_zero',
        'NZ': 'ot_not_zero',
        'C':  'ot_carry',
        'NC': 'ot_not_carry',
        }

@dataclass
class Opcode:
    mnemonic: str
    bytes: int
    cycles: [int]
    operands: [dict]
    immediate: bool
    flags: dict


def c_init(x):
    x = list(x)
    empty = "{\"\", false, 0}"
    for i in range(3):
        x.append(empty)

    body = ', '.join(x[:3])

    return '{' + body + '}'


def operand_to_c(x):
    name = x.get('name', "").lower()
    immediate = str(x['immediate']).lower()
    n_bytes = x.get('bytes', 0)
    return f"{{\"{name}\", {immediate}, {n_bytes}}}"


def main():
    with open("./src/gb-opcodes/Opcodes.json") as f:
        json_opcodes = json.load(f)
    #pprint(opcodes)

    unprefixed = {int(k, 16): Opcode(**v) for (k, v) in json_opcodes['unprefixed'].items()}
    cbprefixed = {int(k, 16): Opcode(**v) for (k, v) in json_opcodes['cbprefixed'].items()}

    with open(sys.argv[1], 'w') as f:
        f.write(dedent("""
        typedef struct Operand {
            char name[4];
            int immediate;
            int bytes;
        } Operand;

        typedef struct Opcode_Flags {
            char z;
            char n;
            char h;
            char c;
        } Opcode_Flags;

        typedef struct Opcode {
            int code;
            char mnemonic[16];
            int bytes;
            int total_cycles;
            int cycles[2];
            int num_operands;
            Operand operands[3];
            int immediate;
            Opcode_Flags flag;
        } Opcode;

        """).strip())

        f.write("\n\nOpcode unprefixed[256] = {\n")

        ops = []
        for k, v in unprefixed.items():
            total_cycles = sum(v.cycles)
            cycles    = '{' + ', '.join(str(c) for c in (v.cycles + [0])[:2]) + '}'
            operands  = c_init(map(operand_to_c, v.operands))
            flags     = '{' + ', '.join([f"'{f.lower()}'" for f in v.flags.values()]) + '}'
            op = f"{{0x{k:02x}, \"{v.mnemonic.lower()}\", {v.bytes}, {total_cycles}, {cycles}, {len(v.operands)}, {operands}, {str(v.immediate).lower()}, {flags}}}"
            ops.append(op)

        f.write('    ' + ',\n    '.join(ops))
        f.write("\n};\n\n")

    return


if __name__ == "__main__":
    main()

