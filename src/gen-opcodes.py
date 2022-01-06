import sys
import json
from pprint import pprint
from dataclasses import dataclass
from textwrap import dedent

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
    for i in range(2):
        x.append(empty)

    body = ', '.join(x[:2])

    return '{' + body + '}'


def operand_to_c(x):
    name = x.get('name', "").lower()
    immediate = str(x['immediate']).lower()
    n_bytes = x.get('bytes', 0)
    return f"{{\"{name}\", {immediate}, {n_bytes}}}"


def escape_keyword(head, kw):
    kmap = {
            'd8': 'u8',
            'd16': 'u16',
            }
    if head in ['call', 'ret', 'jp', 'jr']:
        kmap.update({
            'c': 'flag_cy',
            'nc': 'flag_nc',
            'z': 'flag_z',
            'nz': 'flag_nz',
            })

    if kw.startswith('illegal'):
        kw = 'illegal'
    else:
        kw = kmap.get(kw, kw)
    return "keyword_" + kw


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
            Operand operands[2];
            int immediate;
            Opcode_Flags flag;
            int prefixed;
            int num_words;
            Keyword words[4];
        } Opcode;

        """).strip())

        f.write("\n\nOpcode opcode_table[256] = {\n")

        ops = []

        for k, v in unprefixed.items():
            total_cycles = sum(v.cycles)
            cycles    = '{' + ', '.join(str(c) for c in (v.cycles + [0])[:2]) + '}'
            operands  = c_init(map(operand_to_c, v.operands))
            flags     = '{' + ', '.join([f"'{f.lower()}'" for f in v.flags.values()]) + '}'
            keywords = []
            k1 = v.mnemonic.lower()
            keywords.append(k1)
            keywords.extend(operand['name'].lower() for operand in v.operands)
            num_keywords = len(keywords)
            keywords.extend(['nil', 'nil', 'nil'])
            keywords = [escape_keyword(keywords[0], kw) for kw in  keywords[:4]]
            keywords_string = "{" + ', '.join(keywords) + "}"
            op = f"{{0x{k:02x}, \"{v.mnemonic.lower()}\", {v.bytes}, {total_cycles}, {cycles}, {len(v.operands)}, {operands}, {str(v.immediate).lower()}, {flags}, false, {num_keywords}, {keywords_string}}}"
            ops.append(op)

        f.write('    ' + ',\n    '.join(ops))
        f.write("\n};\n\n")


    return


if __name__ == "__main__":
    main()

