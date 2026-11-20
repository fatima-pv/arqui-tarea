#!/usr/bin/env python3
"""Envuelve un .s ensamblado con rvasm.py en un ELF32 RISC-V mínimo (solo para
probar el cargador ELF del simulador sin tener el toolchain de RISC-V).

Uso: python3 tools/mkelf.py programa.s -o programa.elf [--base 0x10000]
El código se ensambla asumiendo dirección 0 y luego se reubica en --base;
como RISC-V usa saltos relativos al PC, el resultado es correcto siempre que
el programa no use direcciones absolutas de etiquetas.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rvasm import Assembler  # noqa: E402


def build(code, base, labels):
    # Diseño: [ELF header][program header][código][.strtab][.symtab][.shstrtab][section headers]
    ehsize, phsize, shsize = 52, 32, 40
    code_off = ehsize + phsize
    strtab = b"\0"
    syms = [struct.pack("<IIIBBH", 0, 0, 0, 0, 0, 0)]
    for name, addr in sorted(labels.items(), key=lambda kv: kv[1]):
        syms.append(struct.pack("<IIIBBH", len(strtab), base + addr, 0, 0x10 | 2, 0, 1))  # GLOBAL FUNC, sección 1
        strtab += name.encode() + b"\0"
    symtab = b"".join(syms)
    shstr = b"\0.text\0.strtab\0.symtab\0.shstrtab\0"
    strtab_off = code_off + len(code)
    symtab_off = strtab_off + len(strtab)
    shstr_off = symtab_off + len(symtab)
    sh_off = shstr_off + len(shstr)

    eh = b"\x7fELF" + bytes([1, 1, 1, 0]) + bytes(8)
    eh += struct.pack("<HHIIIIIHHHHHH", 2, 243, 1, base, ehsize, sh_off, 0,
                      ehsize, phsize, 1, shsize, 5, 4)
    ph = struct.pack("<IIIIIIII", 1, code_off, base, base, len(code), len(code), 5, 4)
    sh = bytes(shsize)
    sh += struct.pack("<IIIIIIIIII", 1, 1, 6, base, code_off, len(code), 0, 0, 4, 0)            # .text
    sh += struct.pack("<IIIIIIIIII", 7, 3, 0, 0, strtab_off, len(strtab), 0, 0, 1, 0)          # .strtab
    sh += struct.pack("<IIIIIIIIII", 15, 2, 0, 0, symtab_off, len(symtab), 2, 1, 4, 16)        # .symtab
    sh += struct.pack("<IIIIIIIIII", 23, 3, 0, 0, shstr_off, len(shstr), 0, 0, 1, 0)           # .shstrtab
    return eh + ph + code + strtab + symtab + shstr + sh


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--base", default="0x10000")
    a = ap.parse_args()
    asm = Assembler()
    code, _ = asm.assemble(open(a.src).read())
    with open(a.out, "wb") as f:
        f.write(build(code, int(a.base, 0), asm.labels))
    print(f"{a.out}: ELF32 RISC-V, entry {a.base}")


if __name__ == "__main__":
    main()
