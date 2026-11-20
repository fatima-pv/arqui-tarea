#pragma once
// Estado arquitectural de una CPU RV32I (+ extensión M) y su ciclo
// fetch -> decode -> execute.

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "memory.hpp"

enum class StepStatus {
    Ok,           // instrucción ejecutada normalmente
    Exited,       // el programa terminó (ecall exit)
    SelfLoop,     // salto a sí mismo (p.ej. "j ." / "done: beq x2,x2,done")
    Breakpoint,   // instrucción ebreak
    Error,        // instrucción inválida, PC desalineado, etc.
};

struct StepResult {
    StepStatus status = StepStatus::Ok;
    std::string message;  // detalle en caso de error / salida
};

// Entrada/salida usada por las environment calls (ecall).
struct SyscallIO {
    std::function<void(const std::string&)> out;
    std::function<bool(std::string&)> read_line;  // false si EOF
};

class Cpu {
public:
    explicit Cpu(Memory& mem) : mem_(mem) { reset(); }

    void reset(uint32_t entry = 0) {
        regs_.fill(0);
        pc_ = entry;
        exited_ = false;
        exit_code_ = 0;
        instret_ = 0;
    }

    // Ejecuta exactamente una instrucción.
    StepResult step();

    uint32_t pc() const { return pc_; }
    void set_pc(uint32_t pc) { pc_ = pc; }

    uint32_t reg(unsigned i) const { return regs_[i & 31]; }
    void set_reg(unsigned i, uint32_t v) {
        if ((i & 31) != 0) regs_[i & 31] = v;  // x0 siempre vale 0
    }

    bool exited() const { return exited_; }
    int exit_code() const { return exit_code_; }
    uint64_t instret() const { return instret_; }

    void set_io(SyscallIO io) { io_ = std::move(io); }

private:
    StepResult ecall();
    StepResult illegal(uint32_t inst, const char* why = "instrucción inválida");

    Memory& mem_;
    std::array<uint32_t, 32> regs_{};
    uint32_t pc_ = 0;
    bool exited_ = false;
    int exit_code_ = 0;
    uint64_t instret_ = 0;
    SyscallIO io_;
};

// Utilidades de decodificación compartidas con el desensamblador.
namespace rv {
inline uint32_t opcode(uint32_t i) { return i & 0x7F; }
inline uint32_t rd(uint32_t i) { return (i >> 7) & 0x1F; }
inline uint32_t funct3(uint32_t i) { return (i >> 12) & 0x7; }
inline uint32_t rs1(uint32_t i) { return (i >> 15) & 0x1F; }
inline uint32_t rs2(uint32_t i) { return (i >> 20) & 0x1F; }
inline uint32_t funct7(uint32_t i) { return i >> 25; }

inline int32_t imm_i(uint32_t i) { return static_cast<int32_t>(i) >> 20; }
inline int32_t imm_s(uint32_t i) {
    return ((static_cast<int32_t>(i) >> 25) << 5) | static_cast<int32_t>((i >> 7) & 0x1F);
}
inline int32_t imm_b(uint32_t i) {
    return (static_cast<int32_t>(i & 0x80000000u) >> 19) |
           static_cast<int32_t>((i & 0x80) << 4) |
           static_cast<int32_t>((i >> 20) & 0x7E0) |
           static_cast<int32_t>((i >> 7) & 0x1E);
}
inline int32_t imm_u(uint32_t i) { return static_cast<int32_t>(i & 0xFFFFF000u); }
inline int32_t imm_j(uint32_t i) {
    return (static_cast<int32_t>(i & 0x80000000u) >> 11) |
           static_cast<int32_t>(i & 0xFF000) |
           static_cast<int32_t>((i >> 9) & 0x800) |
           static_cast<int32_t>((i >> 20) & 0x7FE);
}

enum Opcode : uint32_t {
    OP_LOAD = 0x03,
    OP_MISC_MEM = 0x0F,
    OP_IMM = 0x13,
    OP_AUIPC = 0x17,
    OP_STORE = 0x23,
    OP_REG = 0x33,
    OP_LUI = 0x37,
    OP_BRANCH = 0x63,
    OP_JALR = 0x67,
    OP_JAL = 0x6F,
    OP_SYSTEM = 0x73,
};
}  // namespace rv
