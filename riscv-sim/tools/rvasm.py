#!/usr/bin/env python3
"""Mini-ensamblador RV32I(+M) usado SOLO para generar los binarios de prueba.

No es parte del simulador (la tarea no pide un ensamblador). Existe porque no
hay toolchain RISC-V instalado; los .bin que genera son equivalentes a los que
exporta CPUlator con "Format: Raw" desde la dirección 0.

Uso:  python3 tools/rvasm.py programa.s -o programa.bin [--list]
"""
import argparse
import re
import sys

REGS = {f"x{i}": i for i in range(32)}
ABI = ["zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3",
       "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
       "t3", "t4", "t5", "t6"]
REGS.update({n: i for i, n in enumerate(ABI)})
REGS["fp"] = 8

R_OPS = {  # nombre: (funct7, funct3)
    "add": (0, 0), "sub": (0x20, 0), "sll": (0, 1), "slt": (0, 2), "sltu": (0, 3),
    "xor": (0, 4), "srl": (0, 5), "sra": (0x20, 5), "or": (0, 6), "and": (0, 7),
    "mul": (1, 0), "mulh": (1, 1), "mulhsu": (1, 2), "mulhu": (1, 3),
    "div": (1, 4), "divu": (1, 5), "rem": (1, 6), "remu": (1, 7),
}
I_OPS = {"addi": 0, "slti": 2, "sltiu": 3, "xori": 4, "ori": 6, "andi": 7}
SH_OPS = {"slli": (0, 1), "srli": (0, 5), "srai": (0x20, 5)}
LOADS = {"lb": 0, "lh": 1, "lw": 2, "lbu": 4, "lhu": 5}
STORES = {"sb": 0, "sh": 1, "sw": 2}
BRANCHES = {"beq": 0, "bne": 1, "blt": 4, "bge": 5, "bltu": 6, "bgeu": 7}


class AsmError(Exception):
    pass


def reg(t):
    t = t.strip()
    if t not in REGS:
        raise AsmError(f"registro inválido: {t}")
    return REGS[t]


def r_type(f7, rs2, rs1, f3, rd, op):
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op


def i_type(imm, rs1, f3, rd, op):
    check(imm, -2048, 2047)
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op


def s_type(imm, rs2, rs1, f3, op):
    check(imm, -2048, 2047)
    imm &= 0xFFF
    return ((imm >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((imm & 0x1F) << 7) | op


def b_type(off, rs2, rs1, f3):
    check(off, -4096, 4094)
    o = off & 0x1FFF
    return (((o >> 12) & 1) << 31) | (((o >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) | \
        (f3 << 12) | (((o >> 1) & 0xF) << 8) | (((o >> 11) & 1) << 7) | 0x63


def j_type(off, rd):
    check(off, -(1 << 20), (1 << 20) - 2)
    o = off & 0x1FFFFF
    return (((o >> 20) & 1) << 31) | (((o >> 1) & 0x3FF) << 21) | (((o >> 11) & 1) << 20) | \
        (((o >> 12) & 0xFF) << 12) | (rd << 7) | 0x6F


def u_type(imm20, rd, op):
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | op


def check(v, lo, hi):
    if not lo <= v <= hi:
        raise AsmError(f"inmediato fuera de rango: {v} (permitido {lo}..{hi})")


def hi_lo(v):
    v &= 0xFFFFFFFF
    hi = ((v + 0x800) >> 12) & 0xFFFFF
    lo = v - (hi << 12)
    lo = ((lo + 0x800) & 0xFFF) - 0x800
    return hi, lo


class Assembler:
    def __init__(self):
        self.labels = {}

    def num(self, t):
        t = t.strip()
        if t in self.labels:
            return self.labels[t]
        if len(t) == 3 and t[0] == t[2] == "'":
            return ord(t[1])
        try:
            return int(t, 0)
        except ValueError:
            raise AsmError(f"valor inválido: {t}")

    def mem_operand(self, t):
        m = re.fullmatch(r"\s*(.*?)\s*\(\s*(\w+)\s*\)\s*", t)
        if not m:
            raise AsmError(f"operando de memoria inválido: {t}")
        return (self.num(m.group(1)) if m.group(1) else 0), reg(m.group(2))

    # Tamaño en bytes de una instrucción (pseudo-instrucciones pueden ser 2).
    def size(self, mn, ops):
        if mn == "li":
            v = int(ops[1], 0)
            return 4 if -2048 <= v <= 2047 else 8
        if mn == "la":
            return 8
        return 4

    def encode(self, mn, ops, pc):
        n = len(ops)
        if mn in R_OPS:
            f7, f3 = R_OPS[mn]
            return [r_type(f7, reg(ops[2]), reg(ops[1]), f3, reg(ops[0]), 0x33)]
        if mn in I_OPS:
            return [i_type(self.num(ops[2]), reg(ops[1]), I_OPS[mn], reg(ops[0]), 0x13)]
        if mn in SH_OPS:
            f7, f3 = SH_OPS[mn]
            sh = self.num(ops[2])
            check(sh, 0, 31)
            return [r_type(f7, sh, reg(ops[1]), f3, reg(ops[0]), 0x13)]
        if mn in LOADS:
            imm, rs1 = self.mem_operand(ops[1])
            return [i_type(imm, rs1, LOADS[mn], reg(ops[0]), 0x03)]
        if mn in STORES:
            imm, rs1 = self.mem_operand(ops[1])
            return [s_type(imm, reg(ops[0]), rs1, STORES[mn], 0x23)]
        if mn in BRANCHES:
            return [b_type(self.num(ops[2]) - pc, reg(ops[1]), reg(ops[0]), BRANCHES[mn])]
        if mn == "lui":
            return [u_type(self.num(ops[1]), reg(ops[0]), 0x37)]
        if mn == "auipc":
            return [u_type(self.num(ops[1]), reg(ops[0]), 0x17)]
        if mn == "jal":
            if n == 1:
                return [j_type(self.num(ops[0]) - pc, 1)]
            return [j_type(self.num(ops[1]) - pc, reg(ops[0]))]
        if mn == "jalr":
            if n == 1:
                return [i_type(0, reg(ops[0]), 0, 1, 0x67)]
            if n == 2:
                imm, rs1 = self.mem_operand(ops[1])
                return [i_type(imm, rs1, 0, reg(ops[0]), 0x67)]
            return [i_type(self.num(ops[2]), reg(ops[1]), 0, reg(ops[0]), 0x67)]
        if mn == "ecall":
            return [0x00000073]
        if mn == "ebreak":
            return [0x00100073]
        if mn == "fence":
            return [0x0FF0000F]
        # ---- pseudo-instrucciones ----
        if mn == "nop":
            return [0x00000013]
        if mn == "li":
            rd, v = reg(ops[0]), self.num(ops[1])
            if -2048 <= v <= 2047:
                return [i_type(v, 0, 0, rd, 0x13)]
            hi, lo = hi_lo(v)
            return [u_type(hi, rd, 0x37), i_type(lo, rd, 0, rd, 0x13)]
        if mn == "la":
            rd = reg(ops[0])
            hi, lo = hi_lo(self.num(ops[1]) - pc)
            return [u_type(hi, rd, 0x17), i_type(lo, rd, 0, rd, 0x13)]
        if mn == "mv":
            return [i_type(0, reg(ops[1]), 0, reg(ops[0]), 0x13)]
        if mn == "not":
            return [i_type(-1, reg(ops[1]), 4, reg(ops[0]), 0x13)]
        if mn == "neg":
            return [r_type(0x20, reg(ops[1]), 0, 0, reg(ops[0]), 0x33)]
        if mn == "seqz":
            return [i_type(1, reg(ops[1]), 3, reg(ops[0]), 0x13)]
        if mn == "snez":
            return [r_type(0, reg(ops[1]), 0, 3, reg(ops[0]), 0x33)]
        if mn == "j":
            return [j_type(self.num(ops[0]) - pc, 0)]
        if mn == "jr":
            return [i_type(0, reg(ops[0]), 0, 0, 0x67)]
        if mn == "ret":
            return [i_type(0, 1, 0, 0, 0x67)]
        if mn == "call":
            return [j_type(self.num(ops[0]) - pc, 1)]
        zero_br = {"beqz": ("beq", 0), "bnez": ("bne", 0), "bltz": ("blt", 0), "bgez": ("bge", 0),
                   "blez": ("bge", 1), "bgtz": ("blt", 1)}
        if mn in zero_br:
            base, swap = zero_br[mn]
            a, b = (ops[0], "zero") if not swap else ("zero", ops[0])
            return self.encode(base, [a, b, ops[1]], pc)
        swapped = {"bgt": "blt", "ble": "bge", "bgtu": "bltu", "bleu": "bgeu"}
        if mn in swapped:
            return self.encode(swapped[mn], [ops[1], ops[0], ops[2]], pc)
        raise AsmError(f"instrucción desconocida: {mn}")

    def parse(self, text):
        """Devuelve una lista de (línea, addr, tipo, datos)."""
        items = []
        for lineno, raw in enumerate(text.splitlines(), 1):
            line = raw.split("#")[0].strip()
            while True:
                m = re.match(r"^([A-Za-z_.$][\w.$]*)\s*:\s*(.*)$", line)
                if not m:
                    break
                items.append((lineno, "label", m.group(1), None))
                line = m.group(2).strip()
            if not line:
                continue
            if line.startswith("."):
                parts = line.split(None, 1)
                items.append((lineno, "dir", parts[0], parts[1] if len(parts) > 1 else ""))
                continue
            parts = line.split(None, 1)
            ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
            items.append((lineno, "ins", parts[0].lower(), ops))
        return items

    def directive_bytes(self, name, arg, addr, resolve):
        if name in (".text", ".data", ".globl", ".global", ".section", ".type", ".size"):
            return b""
        if name == ".org":
            target = int(arg, 0)
            if target < addr:
                raise AsmError(".org hacia atrás")
            return bytes(target - addr)
        if name == ".align":
            a = 1 << int(arg, 0)
            return bytes((-addr) % a)
        if name == ".space":
            return bytes(int(arg, 0))
        if name in (".asciz", ".string", ".ascii"):
            s = bytes(arg.strip()[1:-1], "utf-8").decode("unicode_escape").encode("latin-1")
            return s + (b"\0" if name != ".ascii" else b"")
        sizes = {".word": 4, ".half": 2, ".byte": 1}
        if name in sizes:
            sz = sizes[name]
            out = b""
            for v in arg.split(","):
                val = self.num(v) if resolve else 0
                out += (val & ((1 << (8 * sz)) - 1)).to_bytes(sz, "little")
            return out
        raise AsmError(f"directiva no soportada: {name}")

    def assemble(self, text):
        items = self.parse(text)
        # Pasada 1: direcciones de etiquetas.
        addr = 0
        for lineno, kind, a, b in items:
            try:
                if kind == "label":
                    self.labels[a] = addr
                elif kind == "dir":
                    addr += len(self.directive_bytes(a, b, addr, resolve=False))
                else:
                    addr += self.size(a, b)
            except (AsmError, ValueError, IndexError) as e:
                raise AsmError(f"línea {lineno}: {e}")
        # Pasada 2: codificación.
        out = bytearray()
        listing = []
        for lineno, kind, a, b in items:
            try:
                if kind == "dir":
                    out += self.directive_bytes(a, b, len(out), resolve=True)
                elif kind == "ins":
                    pc = len(out)
                    for w in self.encode(a, b, pc):
                        listing.append((len(out), w, f"{a} {', '.join(b)}"))
                        out += w.to_bytes(4, "little")
            except (AsmError, ValueError, IndexError) as e:
                raise AsmError(f"línea {lineno}: {e}")
        return bytes(out), listing


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--list", action="store_true", help="imprime el listado dirección/código")
    args = ap.parse_args()
    with open(args.src) as f:
        text = f.read()
    try:
        data, listing = Assembler().assemble(text)
    except AsmError as e:
        sys.exit(f"{args.src}: {e}")
    with open(args.out, "wb") as f:
        f.write(data)
    if args.list:
        for addr, w, src in listing:
            print(f"{addr:08x}: {w:08x}  {src}")
    print(f"{args.out}: {len(data)} bytes")


if __name__ == "__main__":
    main()
