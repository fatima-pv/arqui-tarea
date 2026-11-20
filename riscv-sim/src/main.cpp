// rvsim: simulador del ISA RV32I (+M) con interfaz de línea de comandos.
//
//   ./build/rvsim programa.bin          (modo interactivo)
//   ./build/rvsim programa.bin --run    (ejecuta hasta terminar y muestra el estado)

#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "cpu.hpp"
#include "disasm.hpp"
#include "loader.hpp"

namespace {

std::string hex32(uint32_t v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%08X", v);
    return buf;
}

std::vector<std::string> split(const std::string& s) {
    std::istringstream is(s);
    std::vector<std::string> out;
    std::string w;
    while (is >> w) out.push_back(w);
    return out;
}

class Simulator {
public:
    Simulator() : cpu_(mem_) {
        cpu_.set_io({
            [this](const std::string& s) {
                std::cout << s << std::flush;
                if (!s.empty()) last_output_newline_ = s.back() == '\n';
            },
            [this](std::string& line) {
                flush_program_output();
                return static_cast<bool>(std::getline(std::cin, line));
            },
        });
    }

    bool load(const std::string& path) {
        LoadedProgram p = load_program(path, mem_);
        if (!p.ok) {
            std::cout << "Error: " << p.error << "\n";
            return false;
        }
        path_ = path;
        prog_ = p;
        cpu_.reset(p.entry);
        halted_ = false;
        std::cout << "\"" << path << "\" cargado a memoria (" << p.format << ", "
                  << (p.end - p.start) << " bytes en " << hex32(p.start) << "-" << hex32(p.end - 1)
                  << ", pc = " << hex32(p.entry) << ").\n";
        return true;
    }

    // Ejecuta un comando; devuelve false si hay que salir.
    bool command(const std::string& line);

    void run_to_end(uint64_t max_steps) {
        run(max_steps, false);
    }
    void print_all_regs();
    int exit_code() const { return cpu_.exit_code(); }

private:
    // --- ejecución ---
    StepResult step_one(bool verbose);
    void run(uint64_t max_steps, bool from_user);
    void report_stop(const StepResult& r);

    // --- utilidades ---
    bool parse_value(const std::string& tok, uint32_t& out) const;
    std::string label_at(uint32_t addr) const;
    void disasm_range(uint32_t addr, unsigned count);
    void dump_memory(uint32_t start, uint32_t end);
    void flush_program_output() {
        if (!last_output_newline_) {
            std::cout << "\n";
            last_output_newline_ = true;
        }
    }
    void help();

    Memory mem_;
    Cpu cpu_;
    LoadedProgram prog_;
    std::string path_;
    std::set<uint32_t> breakpoints_;
    bool halted_ = false;
    bool trace_ = false;
    bool last_output_newline_ = true;
};

bool Simulator::parse_value(const std::string& tok, uint32_t& out) const {
    if (tok.empty()) return false;
    int r = parse_reg_name(tok);
    if (r >= 0) {
        out = cpu_.reg(static_cast<unsigned>(r));
        return true;
    }
    if (tok == "pc") {
        out = cpu_.pc();
        return true;
    }
    for (const auto& [addr, name] : prog_.symbols) {
        if (name == tok) {
            out = addr;
            return true;
        }
    }
    try {
        size_t used = 0;
        long long v = std::stoll(tok, &used, 0);
        if (used != tok.size()) return false;
        out = static_cast<uint32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

std::string Simulator::label_at(uint32_t addr) const {
    auto it = prog_.symbols.find(addr);
    return it == prog_.symbols.end() ? "" : it->second;
}

StepResult Simulator::step_one(bool verbose) {
    const uint32_t pc = cpu_.pc();
    const uint32_t inst = mem_.read32(pc);

    std::array<uint32_t, 32> before{};
    for (unsigned i = 0; i < 32; ++i) before[i] = cpu_.reg(i);

    // Si es un store, recordamos la dirección para mostrar lo escrito.
    bool is_store = rv::opcode(inst) == rv::OP_STORE;
    uint32_t store_addr = cpu_.reg(rv::rs1(inst)) + static_cast<uint32_t>(rv::imm_s(inst));
    unsigned store_size = 1u << (rv::funct3(inst) & 3);

    StepResult r = cpu_.step();

    if (verbose) {
        flush_program_output();
        std::string lbl = label_at(pc);
        if (!lbl.empty()) std::cout << lbl << ":\n";
        char buf[160];
        std::snprintf(buf, sizeof buf, "  [%s] %08x  %s", hex32(pc).c_str(), inst,
                      disassemble(inst, pc).c_str());
        std::cout << buf << "\n";
        if (r.status != StepStatus::Error) {
            for (unsigned i = 1; i < 32; ++i) {
                if (cpu_.reg(i) != before[i]) {
                    std::snprintf(buf, sizeof buf, "      x%-2u (%-4s): %s -> %s  (%d)", i,
                                  reg_abi_name(i), hex32(before[i]).c_str(),
                                  hex32(cpu_.reg(i)).c_str(), static_cast<int32_t>(cpu_.reg(i)));
                    std::cout << buf << "\n";
                }
            }
            if (is_store) {
                std::cout << "      mem[" << hex32(store_addr) << "] <-";
                for (unsigned k = 0; k < store_size; ++k) {
                    std::snprintf(buf, sizeof buf, " %02X", mem_.read8(store_addr + k));
                    std::cout << buf;
                }
                std::cout << "\n";
            }
        }
    }
    return r;
}

void Simulator::report_stop(const StepResult& r) {
    flush_program_output();
    switch (r.status) {
        case StepStatus::Ok:
            break;
        case StepStatus::Exited:
            halted_ = true;
            std::cout << "Programa terminado con ecall " << r.message << " tras "
                      << cpu_.instret() << " instrucciones.\n";
            break;
        case StepStatus::SelfLoop:
            halted_ = true;
            std::cout << "Programa terminado: " << r.message << " tras " << cpu_.instret()
                      << " instrucciones.\n";
            break;
        case StepStatus::Breakpoint:
            std::cout << "ebreak alcanzado; pc = " << hex32(cpu_.pc()) << "\n";
            break;
        case StepStatus::Error:
            halted_ = true;
            std::cout << "Error de ejecución: " << r.message << "\n";
            break;
    }
}

void Simulator::run(uint64_t max_steps, bool from_user) {
    if (halted_) {
        std::cout << "El programa ya terminó. Usa 'reset' para reiniciarlo.\n";
        return;
    }
    for (uint64_t n = 0; n < max_steps; ++n) {
        if (n > 0 && breakpoints_.count(cpu_.pc())) {
            flush_program_output();
            std::cout << "Breakpoint en " << hex32(cpu_.pc()) << "  "
                      << disassemble(mem_.read32(cpu_.pc()), cpu_.pc()) << "\n";
            return;
        }
        StepResult r = step_one(trace_);
        if (r.status != StepStatus::Ok) {
            report_stop(r);
            return;
        }
    }
    flush_program_output();
    std::cout << "Se alcanzó el límite de " << max_steps << " instrucciones (pc = "
              << hex32(cpu_.pc()) << ")." << (from_user ? " Usa 'run' de nuevo para continuar." : "")
              << "\n";
}

void Simulator::print_all_regs() {
    char buf[64];
    std::cout << "pc = " << hex32(cpu_.pc()) << "\n";
    for (unsigned row = 0; row < 8; ++row) {
        for (unsigned col = 0; col < 4; ++col) {
            unsigned i = col * 8 + row;
            std::snprintf(buf, sizeof buf, "x%-2u %-4s %s   ", i, reg_abi_name(i), hex32(cpu_.reg(i)).c_str());
            std::cout << buf;
        }
        std::cout << "\n";
    }
}

void Simulator::disasm_range(uint32_t addr, unsigned count) {
    addr &= ~3u;
    for (unsigned k = 0; k < count; ++k, addr += 4) {
        std::string lbl = label_at(addr);
        if (!lbl.empty()) std::cout << lbl << ":\n";
        uint32_t inst = mem_.read32(addr);
        char buf[160];
        std::snprintf(buf, sizeof buf, "%s%s %s: %08x  %s", addr == cpu_.pc() ? "=>" : "  ",
                      breakpoints_.count(addr) ? "*" : " ", hex32(addr).c_str(), inst,
                      disassemble(inst, addr).c_str());
        std::cout << buf << "\n";
    }
}

void Simulator::dump_memory(uint32_t start, uint32_t end) {
    char buf[16];
    if (end < start) std::swap(start, end);
    if (end - start < 16) {
        std::cout << "Memoria (" << hex32(start) << "-" << hex32(end) << "):";
        for (uint32_t a = start;; ++a) {
            std::snprintf(buf, sizeof buf, " %02X", mem_.read8(a));
            std::cout << buf;
            if (a == end) break;
        }
        std::cout << "\n";
        return;
    }
    uint64_t last = std::min<uint64_t>(end, static_cast<uint64_t>(start) + 4095);  // máx. 4 KiB
    for (uint64_t row = start & ~15ull; row <= last; row += 16) {
        std::cout << hex32(static_cast<uint32_t>(row)) << ":";
        std::string ascii;
        for (unsigned k = 0; k < 16; ++k) {
            uint64_t a = row + k;
            if (a < start || a > last) {
                std::cout << "   ";
                ascii += ' ';
                continue;
            }
            uint8_t v = mem_.read8(static_cast<uint32_t>(a));
            std::snprintf(buf, sizeof buf, " %02X", v);
            std::cout << buf << (k == 7 ? " " : "");
            ascii += (v >= 32 && v < 127) ? static_cast<char>(v) : '.';
        }
        std::cout << "  |" << ascii << "|\n";
    }
    if (last < end) std::cout << "(se muestran solo los primeros 4096 bytes)\n";
}

void Simulator::help() {
    std::cout <<
        "Comandos disponibles:\n"
        "  step [n]            (s)  ejecuta 1 (o n) instrucciones mostrando cada una\n"
        "  run [max]           (r)  ejecuta hasta terminar, breakpoint o max instrucciones\n"
        "  pc                       muestra el contador de programa\n"
        "  regs [r1 r2 ...]         muestra todos los registros o solo los indicados (x5, a0, sp...)\n"
        "  mem <ini> [fin]          muestra bytes de memoria (rango inclusivo)\n"
        "  memw <dir> [n]           muestra n palabras de 32 bits (hex y decimal)\n"
        "  disasm [dir] [n]    (d)  desensambla n instrucciones desde dir (por defecto pc)\n"
        "  break <dir>         (b)  pone un breakpoint;  delete <dir|all> lo quita\n"
        "  breaks                   lista los breakpoints\n"
        "  set <reg|pc> <valor>     cambia un registro o el pc\n"
        "  poke <dir> <valor>       escribe una palabra de 32 bits en memoria\n"
        "  trace on|off             muestra cada instrucción durante 'run'\n"
        "  info                     información del programa cargado\n"
        "  reset                    recarga el programa y reinicia la CPU\n"
        "  load <archivo>           carga otro programa (.bin crudo o .elf)\n"
        "  help                (h)  esta ayuda\n"
        "  exit                (q)  salir\n"
        "Los valores aceptan decimal, hexadecimal (0x...), nombres de registro y etiquetas ELF.\n"
        "Una línea vacía repite el último comando.\n";
}

bool Simulator::command(const std::string& line) {
    auto args = split(line);
    if (args.empty()) return true;
    const std::string cmd = args[0];
    auto need_loaded = [&]() {
        if (path_.empty()) {
            std::cout << "No hay programa cargado. Usa 'load <archivo>'.\n";
            return false;
        }
        return true;
    };
    auto value = [&](size_t i, uint32_t& out) {
        if (i >= args.size() || !parse_value(args[i], out)) {
            std::cout << "Valor inválido: '" << (i < args.size() ? args[i] : "") << "'\n";
            return false;
        }
        return true;
    };

    if (cmd == "exit" || cmd == "quit" || cmd == "q") {
        std::cout << "See you next time...\n";
        return false;
    } else if (cmd == "help" || cmd == "h" || cmd == "?") {
        help();
    } else if (cmd == "pc") {
        std::cout << "pc = " << hex32(cpu_.pc()) << "\n";
    } else if (cmd == "step" || cmd == "s" || cmd == "si") {
        if (!need_loaded()) return true;
        uint32_t n = 1;
        if (args.size() > 1 && !value(1, n)) return true;
        if (halted_) {
            std::cout << "El programa ya terminó. Usa 'reset' para reiniciarlo.\n";
            return true;
        }
        for (uint32_t k = 0; k < n; ++k) {
            StepResult r = step_one(true);
            if (r.status != StepStatus::Ok) {
                report_stop(r);
                break;
            }
        }
    } else if (cmd == "run" || cmd == "r" || cmd == "continue" || cmd == "c") {
        if (!need_loaded()) return true;
        uint32_t max = 10'000'000;
        if (args.size() > 1 && !value(1, max)) return true;
        run(max, true);
    } else if (cmd == "regs" || cmd == "reg") {
        if (args.size() == 1) {
            print_all_regs();
        } else {
            for (size_t i = 1; i < args.size(); ++i) {
                if (args[i] == "pc") {
                    std::cout << "pc = " << hex32(cpu_.pc()) << "\n";
                    continue;
                }
                int r = parse_reg_name(args[i]);
                if (r < 0) {
                    std::cout << "Registro desconocido: " << args[i] << "\n";
                    continue;
                }
                uint32_t v = cpu_.reg(static_cast<unsigned>(r));
                std::cout << "x" << r << " (" << reg_abi_name(static_cast<unsigned>(r)) << ") = " << hex32(v)
                          << "  (" << static_cast<int32_t>(v) << ")\n";
            }
        }
    } else if (cmd == "mem" || cmd == "x") {
        uint32_t start, end;
        if (!value(1, start)) return true;
        if (args.size() > 2) {
            if (!value(2, end)) return true;
        } else {
            end = start + 63;
        }
        dump_memory(start, end);
    } else if (cmd == "memw") {
        uint32_t addr, n = 8;
        if (!value(1, addr)) return true;
        if (args.size() > 2 && !value(2, n)) return true;
        char buf[96];
        for (uint32_t k = 0; k < n && k < 1024; ++k) {
            uint32_t a = addr + 4 * k, v = mem_.read32(a);
            std::snprintf(buf, sizeof buf, "%s: %s  %d", hex32(a).c_str(), hex32(v).c_str(),
                          static_cast<int32_t>(v));
            std::cout << buf << "\n";
        }
    } else if (cmd == "disasm" || cmd == "d") {
        uint32_t addr = cpu_.pc(), n = 10;
        if (args.size() > 1 && !value(1, addr)) return true;
        if (args.size() > 2 && !value(2, n)) return true;
        disasm_range(addr, n);
    } else if (cmd == "break" || cmd == "b") {
        uint32_t addr;
        if (!value(1, addr)) return true;
        breakpoints_.insert(addr & ~3u);
        std::cout << "Breakpoint en " << hex32(addr & ~3u) << "\n";
    } else if (cmd == "delete") {
        if (args.size() > 1 && args[1] == "all") {
            breakpoints_.clear();
            std::cout << "Breakpoints eliminados.\n";
            return true;
        }
        uint32_t addr;
        if (!value(1, addr)) return true;
        std::cout << (breakpoints_.erase(addr & ~3u) ? "Breakpoint eliminado.\n" : "No había breakpoint ahí.\n");
    } else if (cmd == "breaks") {
        if (breakpoints_.empty()) std::cout << "No hay breakpoints.\n";
        for (uint32_t b : breakpoints_) std::cout << "  " << hex32(b) << "  " << disassemble(mem_.read32(b), b) << "\n";
    } else if (cmd == "set") {
        uint32_t v;
        if (args.size() < 3 || !value(2, v)) {
            std::cout << "Uso: set <reg|pc> <valor>\n";
            return true;
        }
        if (args[1] == "pc") {
            cpu_.set_pc(v);
        } else {
            int r = parse_reg_name(args[1]);
            if (r <= 0) {
                std::cout << "Registro inválido: " << args[1] << "\n";
                return true;
            }
            cpu_.set_reg(static_cast<unsigned>(r), v);
        }
        halted_ = false;
    } else if (cmd == "poke") {
        uint32_t addr, v;
        if (!value(1, addr) || !value(2, v)) return true;
        mem_.write32(addr, v);
    } else if (cmd == "trace") {
        trace_ = args.size() > 1 && args[1] == "on";
        std::cout << "trace " << (trace_ ? "activado" : "desactivado") << "\n";
    } else if (cmd == "info") {
        if (!need_loaded()) return true;
        std::cout << "Programa: " << path_ << " (" << prog_.format << ")\n"
                  << "Rango cargado: " << hex32(prog_.start) << "-" << hex32(prog_.end - 1) << "\n"
                  << "Entry point: " << hex32(prog_.entry) << "\n"
                  << "Instrucciones ejecutadas: " << cpu_.instret() << "\n"
                  << "Estado: " << (halted_ ? "terminado" : "en ejecución") << "\n";
        if (!prog_.symbols.empty()) {
            std::cout << "Símbolos:\n";
            for (const auto& [a, n] : prog_.symbols) std::cout << "  " << hex32(a) << "  " << n << "\n";
        }
    } else if (cmd == "reset") {
        if (!need_loaded()) return true;
        load(path_);
    } else if (cmd == "load") {
        if (args.size() < 2) {
            std::cout << "Uso: load <archivo>\n";
            return true;
        }
        load(args[1]);
    } else {
        std::cout << "Comando desconocido: '" << cmd << "'. Escribe 'help'.\n";
    }
    return true;
}

void usage(const char* argv0) {
    std::cout << "Uso: " << argv0 << " [programa.bin|programa.elf] [opciones]\n"
              << "  -r, --run        ejecuta el programa hasta terminar y muestra los registros\n"
              << "  -t, --trace      (con --run) muestra cada instrucción ejecutada\n"
              << "  -m, --max N      límite de instrucciones para --run (por defecto 10000000)\n"
              << "  -h, --help       muestra esta ayuda\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string path;
    bool run_mode = false, trace = false;
    uint64_t max_steps = 10'000'000;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-r" || a == "--run") run_mode = true;
        else if (a == "-t" || a == "--trace") trace = true;
        else if ((a == "-m" || a == "--max") && i + 1 < argc) max_steps = std::stoull(argv[++i], nullptr, 0);
        else if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "Opción desconocida: " << a << "\n";
            usage(argv[0]);
            return 2;
        } else path = a;
    }

    Simulator sim;
    if (!path.empty() && !sim.load(path)) return 1;

    if (run_mode) {
        if (path.empty()) {
            usage(argv[0]);
            return 2;
        }
        if (trace) sim.command("trace on");
        sim.run_to_end(max_steps);
        sim.print_all_regs();
        return sim.exit_code();
    }

    const bool interactive = isatty(STDIN_FILENO);
    if (interactive) std::cout << "Simulador RISC-V (RV32I). Escribe 'help' para ver los comandos.\n";
    std::string line, last;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }
        if (!interactive) std::cout << line << "\n";  // eco al leer comandos desde un archivo
        if (split(line).empty()) line = last;
        else last = line;
        if (!sim.command(line)) break;
    }
    std::cout << "Program exited with code " << sim.exit_code() << ".\n";
    return 0;
}
