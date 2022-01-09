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
    increment = str(x.get('increment', 'false')).lower()
    decrement = str(x.get('decrement', 'false')).lower()
    n_bytes = x.get('bytes', 0)
    return f"{{\"{name}\", {increment}, {decrement}, {n_bytes}}}"


def escape_keyword(head, kw, immediate=True):
    kmap = {
            'a8': 'u8',
            'a16': 'u16',
            'd8': 'u8',
            'd16': 'u16',
            }
    if head in ['call', 'ret', 'jp', 'jr']:
        kmap.update({
            'c': 'cy',
            'nc': 'nc',
            'z': 'z',
            'nz': 'nz',
            })

    if kw.startswith('illegal'):
        kw = 'illegal'
    else:
        kw = kmap.get(kw, kw)

    if not immediate:
        kw = 'deref_' + kw
    return "keyword_" + kw


def make_keywords(op):
    keywords = []
    k1 = op.mnemonic.lower()
    keywords.append(escape_keyword(None, k1))
    for operand in op.operands:
        k = operand['name'].lower()
        keywords.append(escape_keyword(k1, k, operand['immediate']))
    keywords.extend(['keyword_nil', 'keyword_nil', 'keyword_nil'])
    keywords = keywords[:4]
    return keywords


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
            int increment;
            int decrement;
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
            Opcode_Flags flags;
            int prefixed;
            int num_words;
            Keyword words[4];
        } Opcode;

        """).strip())

        f.write("\n\nOpcode opcode_table[256] = {\n")

        ops = []

        for k, v in unprefixed.items():
            i = any(arg.get('increment', False) for arg in v.operands)
            d = any(arg.get('decrement', False) for arg in v.operands)
            if i:
                v.mnemonic += 'I'
            if d:
                v.mnemonic += 'D'
            total_cycles = sum(v.cycles)
            cycles    = '{' + ', '.join(str(c) for c in (v.cycles + [0])[:2]) + '}'
            operands  = c_init(map(operand_to_c, v.operands))
            flags     = '{' + ', '.join([f"'{f.lower()}'" for f in v.flags.values()]) + '}'
            keywords = make_keywords(v)
            keywords_string = "{" + ', '.join(keywords) + "}"
            num_keywords = len(v.operands) + 1
            op = f"{{0x{k:02x}, \"{v.mnemonic.lower()}\", {v.bytes}, {total_cycles}, {cycles}, {len(v.operands)}, {operands}, {str(v.immediate).lower()}, {flags}, false, {num_keywords}, {keywords_string}}}"
            ops.append(op)

        f.write('    ' + ',\n    '.join(ops))
        f.write("\n};\n\n")

    return


if __name__ == "__main__":
    main()

