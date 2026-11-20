#include "cpu.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>

using namespace rv;

namespace {
std::string hex32(uint32_t v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%08X", v);
    return buf;
}
}

StepResult Cpu::illegal(uint32_t inst, const char* why) {
    return {StepStatus::Error,
            std::string(why) + " " + hex32(inst) + " en pc = " + hex32(pc_)};
}

StepResult Cpu::step() {
    if (exited_) return {StepStatus::Exited, "el programa ya terminó"};
    if (pc_ & 3) return {StepStatus::Error, "PC desalineado: " + hex32(pc_)};

    const uint32_t inst = mem_.read32(pc_);
    uint32_t next_pc = pc_ + 4;

    const uint32_t op = opcode(inst);
    const uint32_t d = rd(inst);
    const uint32_t f3 = funct3(inst);
    const uint32_t f7 = funct7(inst);
    const uint32_t a = regs_[rs1(inst)];
    const uint32_t b = regs_[rs2(inst)];
    const int32_t sa = static_cast<int32_t>(a);
    const int32_t sb = static_cast<int32_t>(b);

    switch (op) {
        case OP_LUI:
            set_reg(d, static_cast<uint32_t>(imm_u(inst)));
            break;

        case OP_AUIPC:
            set_reg(d, pc_ + static_cast<uint32_t>(imm_u(inst)));
            break;

        case OP_JAL:
            set_reg(d, pc_ + 4);
            next_pc = pc_ + static_cast<uint32_t>(imm_j(inst));
            break;

        case OP_JALR: {
            if (f3 != 0) return illegal(inst);
            uint32_t target = (a + static_cast<uint32_t>(imm_i(inst))) & ~1u;
            set_reg(d, pc_ + 4);
            next_pc = target;
            break;
        }

        case OP_BRANCH: {
            bool taken;
            switch (f3) {
                case 0: taken = a == b; break;
                case 1: taken = a != b; break;
                case 4: taken = sa < sb; break;
                case 5: taken = sa >= sb; break;
                case 6: taken = a < b; break;
                case 7: taken = a >= b; break;
                default: return illegal(inst);
            }
            if (taken) next_pc = pc_ + static_cast<uint32_t>(imm_b(inst));
            break;
        }

        case OP_LOAD: {
            uint32_t addr = a + static_cast<uint32_t>(imm_i(inst));
            uint32_t v;
            switch (f3) {
                case 0: v = static_cast<uint32_t>(static_cast<int8_t>(mem_.read8(addr))); break;
                case 1: v = static_cast<uint32_t>(static_cast<int16_t>(mem_.read16(addr))); break;
                case 2: v = mem_.read32(addr); break;
                case 4: v = mem_.read8(addr); break;
                case 5: v = mem_.read16(addr); break;
                default: return illegal(inst);
            }
            set_reg(d, v);
            break;
        }

        case OP_STORE: {
            uint32_t addr = a + static_cast<uint32_t>(imm_s(inst));
            switch (f3) {
                case 0: mem_.write8(addr, static_cast<uint8_t>(b)); break;
                case 1: mem_.write16(addr, static_cast<uint16_t>(b)); break;
                case 2: mem_.write32(addr, b); break;
                default: return illegal(inst);
            }
            break;
        }

        case OP_IMM: {
            const int32_t imm = imm_i(inst);
            const uint32_t uimm = static_cast<uint32_t>(imm);
            const uint32_t shamt = rs2(inst);
            uint32_t v;
            switch (f3) {
                case 0: v = a + uimm; break;
                case 2: v = sa < imm ? 1 : 0; break;
                case 3: v = a < uimm ? 1 : 0; break;
                case 4: v = a ^ uimm; break;
                case 6: v = a | uimm; break;
                case 7: v = a & uimm; break;
                case 1:
                    if (f7 != 0) return illegal(inst);
                    v = a << shamt;
                    break;
                case 5:
                    if (f7 == 0x00) v = a >> shamt;
                    else if (f7 == 0x20) v = static_cast<uint32_t>(sa >> shamt);
                    else return illegal(inst);
                    break;
                default: return illegal(inst);
            }
            set_reg(d, v);
            break;
        }

        case OP_REG: {
            uint32_t v;
            if (f7 == 0x00 || f7 == 0x20) {
                const bool alt = f7 == 0x20;
                switch (f3) {
                    case 0: v = alt ? a - b : a + b; break;
                    case 1: if (alt) return illegal(inst); v = a << (b & 31); break;
                    case 2: if (alt) return illegal(inst); v = sa < sb ? 1 : 0; break;
                    case 3: if (alt) return illegal(inst); v = a < b ? 1 : 0; break;
                    case 4: if (alt) return illegal(inst); v = a ^ b; break;
                    case 5:
                        v = alt ? static_cast<uint32_t>(sa >> (b & 31)) : a >> (b & 31);
                        break;
                    case 6: if (alt) return illegal(inst); v = a | b; break;
                    case 7: if (alt) return illegal(inst); v = a & b; break;
                    default: return illegal(inst);
                }
            } else if (f7 == 0x01) {
                const int64_t A = sa, B = sb;
                const uint64_t UA = a, UB = b;
                switch (f3) {
                    case 0: v = static_cast<uint32_t>(A * B); break;
                    case 1: v = static_cast<uint32_t>(static_cast<uint64_t>(A * B) >> 32); break;
                    case 2: v = static_cast<uint32_t>(static_cast<uint64_t>(A * static_cast<int64_t>(UB)) >> 32); break;
                    case 3: v = static_cast<uint32_t>((UA * UB) >> 32); break;
                    case 4:
                        if (b == 0) v = 0xFFFFFFFFu;
                        else if (sa == INT32_MIN && sb == -1) v = a;
                        else v = static_cast<uint32_t>(sa / sb);
                        break;
                    case 5: v = b == 0 ? 0xFFFFFFFFu : a / b; break;
                    case 6:
                        if (b == 0) v = a;
                        else if (sa == INT32_MIN && sb == -1) v = 0;
                        else v = static_cast<uint32_t>(sa % sb);
                        break;
                    case 7: v = b == 0 ? a : a % b; break;
                    default: return illegal(inst);
                }
            } else {
                return illegal(inst);
            }
            set_reg(d, v);
            break;
        }

        case OP_MISC_MEM:
            break;

        case OP_SYSTEM: {
            if (inst == 0x00000073) {
                StepResult r = ecall();
                ++instret_;
                if (r.status == StepStatus::Ok) pc_ = next_pc;
                return r;
            }
            if (inst == 0x00100073) {
                ++instret_;
                pc_ = next_pc;
                return {StepStatus::Breakpoint, "ebreak"};
            }
            return illegal(inst, "instrucción de sistema no soportada");
        }

        default:
            return illegal(inst);
    }

    ++instret_;
    if (next_pc == pc_) {
        return {StepStatus::SelfLoop, "bucle infinito en " + hex32(pc_) + " (fin del programa)"};
    }
    pc_ = next_pc;
    return {};
}

StepResult Cpu::ecall() {
    const uint32_t a7 = regs_[17];
    const uint32_t a0 = regs_[10];
    const uint32_t a1 = regs_[11];
    auto out = [&](const std::string& s) {
        if (io_.out) io_.out(s);
    };
    auto read_line = [&](std::string& line) {
        return io_.read_line ? io_.read_line(line) : false;
    };

    char buf[64];
    switch (a7) {
        case 1:
            out(std::to_string(static_cast<int32_t>(a0)));
            return {};
        case 4: {
            std::string s;
            for (uint32_t p = a0; s.size() < 1 << 20; ++p) {
                char c = static_cast<char>(mem_.read8(p));
                if (c == '\0') break;
                s += c;
            }
            out(s);
            return {};
        }
        case 5: {
            std::string line;
            if (!read_line(line)) return {StepStatus::Error, "read_int: fin de entrada"};
            set_reg(10, static_cast<uint32_t>(std::strtol(line.c_str(), nullptr, 0)));
            return {};
        }
        case 8: {
            std::string line;
            if (!read_line(line)) line.clear();
            line += '\n';
            if (a1 == 0) return {};
            uint32_t n = 0;
            for (; n + 1 < a1 && n < line.size(); ++n) mem_.write8(a0 + n, static_cast<uint8_t>(line[n]));
            mem_.write8(a0 + n, 0);
            return {};
        }
        case 10:
            exited_ = true;
            exit_code_ = 0;
            return {StepStatus::Exited, "exit"};
        case 11:
            out(std::string(1, static_cast<char>(a0)));
            return {};
        case 12: {
            std::string line;
            if (!read_line(line) || line.empty()) set_reg(10, '\n');
            else set_reg(10, static_cast<uint8_t>(line[0]));
            return {};
        }
        case 17:
        case 93:
            exited_ = true;
            exit_code_ = static_cast<int32_t>(a0);
            return {StepStatus::Exited, "exit(" + std::to_string(exit_code_) + ")"};
        case 34:
            std::snprintf(buf, sizeof buf, "0x%08x", a0);
            out(buf);
            return {};
        case 35: {
            std::string s;
            for (int i = 31; i >= 0; --i) s += ((a0 >> i) & 1) ? '1' : '0';
            out(s);
            return {};
        }
        case 36:
            out(std::to_string(a0));
            return {};
        default: {
            std::ostringstream os;
            os << "ecall no soportada (a7 = " << a7 << ") en pc = " << hex32(pc_);
            return {StepStatus::Error, os.str()};
        }
    }
}
