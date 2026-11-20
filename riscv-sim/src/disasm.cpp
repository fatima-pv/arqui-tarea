#include "disasm.hpp"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "cpu.hpp"

using namespace rv;

static const char* ABI[32] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

const char* reg_abi_name(unsigned i) { return ABI[i & 31]; }

int parse_reg_name(const std::string& name) {
    if (name.size() >= 2 && name[0] == 'x') {
        try {
            size_t used = 0;
            int n = std::stoi(name.substr(1), &used);
            if (used == name.size() - 1 && n >= 0 && n < 32) return n;
        } catch (...) {
        }
        return -1;
    }
    if (name == "fp") return 8;
    for (int i = 0; i < 32; ++i)
        if (name == ABI[i]) return i;
    return -1;
}

namespace {
std::string fmt(const char* f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char* f, ...) {
    char buf[128];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return buf;
}
const char* R(uint32_t i) { return ABI[i & 31]; }
}  // namespace

std::string disassemble(uint32_t inst, uint32_t pc) {
    const uint32_t d = rd(inst), s1 = rs1(inst), s2 = rs2(inst);
    const uint32_t f3 = funct3(inst), f7 = funct7(inst);

    switch (opcode(inst)) {
        case OP_LUI:
            return fmt("lui %s, 0x%x", R(d), static_cast<uint32_t>(imm_u(inst)) >> 12);
        case OP_AUIPC:
            return fmt("auipc %s, 0x%x", R(d), static_cast<uint32_t>(imm_u(inst)) >> 12);
        case OP_JAL: {
            uint32_t t = pc + static_cast<uint32_t>(imm_j(inst));
            if (d == 0) return fmt("j 0x%x", t);
            if (d == 1) return fmt("jal 0x%x", t);
            return fmt("jal %s, 0x%x", R(d), t);
        }
        case OP_JALR: {
            int32_t imm = imm_i(inst);
            if (d == 0 && s1 == 1 && imm == 0) return "ret";
            if (d == 0 && imm == 0) return fmt("jr %s", R(s1));
            return fmt("jalr %s, %d(%s)", R(d), imm, R(s1));
        }
        case OP_BRANCH: {
            static const char* names[8] = {"beq", "bne", nullptr, nullptr, "blt", "bge", "bltu", "bgeu"};
            if (!names[f3]) break;
            uint32_t t = pc + static_cast<uint32_t>(imm_b(inst));
            if (s2 == 0 && f3 == 0) return fmt("beqz %s, 0x%x", R(s1), t);
            if (s2 == 0 && f3 == 1) return fmt("bnez %s, 0x%x", R(s1), t);
            return fmt("%s %s, %s, 0x%x", names[f3], R(s1), R(s2), t);
        }
        case OP_LOAD: {
            static const char* names[8] = {"lb", "lh", "lw", nullptr, "lbu", "lhu", nullptr, nullptr};
            if (!names[f3]) break;
            return fmt("%s %s, %d(%s)", names[f3], R(d), imm_i(inst), R(s1));
        }
        case OP_STORE: {
            static const char* names[8] = {"sb", "sh", "sw", nullptr, nullptr, nullptr, nullptr, nullptr};
            if (!names[f3]) break;
            return fmt("%s %s, %d(%s)", names[f3], R(s2), imm_s(inst), R(s1));
        }
        case OP_IMM: {
            int32_t imm = imm_i(inst);
            switch (f3) {
                case 0:
                    if (inst == 0x00000013) return "nop";
                    if (s1 == 0) return fmt("li %s, %d", R(d), imm);
                    if (imm == 0) return fmt("mv %s, %s", R(d), R(s1));
                    return fmt("addi %s, %s, %d", R(d), R(s1), imm);
                case 2: return fmt("slti %s, %s, %d", R(d), R(s1), imm);
                case 3: return fmt("sltiu %s, %s, %d", R(d), R(s1), imm);
                case 4:
                    if (imm == -1) return fmt("not %s, %s", R(d), R(s1));
                    return fmt("xori %s, %s, %d", R(d), R(s1), imm);
                case 6: return fmt("ori %s, %s, %d", R(d), R(s1), imm);
                case 7: return fmt("andi %s, %s, %d", R(d), R(s1), imm);
                case 1:
                    if (f7 == 0) return fmt("slli %s, %s, %u", R(d), R(s1), s2);
                    break;
                case 5:
                    if (f7 == 0) return fmt("srli %s, %s, %u", R(d), R(s1), s2);
                    if (f7 == 0x20) return fmt("srai %s, %s, %u", R(d), R(s1), s2);
                    break;
            }
            break;
        }
        case OP_REG: {
            const char* name = nullptr;
            if (f7 == 0x00) {
                static const char* n[8] = {"add", "sll", "slt", "sltu", "xor", "srl", "or", "and"};
                name = n[f3];
            } else if (f7 == 0x20) {
                if (f3 == 0) name = "sub";
                if (f3 == 5) name = "sra";
            } else if (f7 == 0x01) {
                static const char* n[8] = {"mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "remu"};
                name = n[f3];
            }
            if (!name) break;
            if (f7 == 0x20 && f3 == 0 && s1 == 0) return fmt("neg %s, %s", R(d), R(s2));
            return fmt("%s %s, %s, %s", name, R(d), R(s1), R(s2));
        }
        case OP_MISC_MEM:
            return f3 == 1 ? "fence.i" : "fence";
        case OP_SYSTEM:
            if (inst == 0x00000073) return "ecall";
            if (inst == 0x00100073) return "ebreak";
            break;
    }
    return fmt(".word 0x%08x   # (inválida)", inst);
}
