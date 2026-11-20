#pragma once
#include <cstdint>
#include <string>

// Nombres de registros (ABI) y parseo de nombres de registro.
const char* reg_abi_name(unsigned i);
// Devuelve el índice 0..31 para "x5", "t0", "fp", "zero", etc.; -1 si no es válido.
int parse_reg_name(const std::string& name);

// Desensambla una instrucción ubicada en `pc` (el pc se usa para calcular
// los destinos absolutos de saltos y branches).
std::string disassemble(uint32_t inst, uint32_t pc);
