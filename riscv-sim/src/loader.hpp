#pragma once
#include <cstdint>
#include <map>
#include <string>

#include "memory.hpp"

struct LoadedProgram {
    bool ok = false;
    std::string error;
    std::string format;        // "binario crudo" o "ELF32"
    uint32_t entry = 0;        // PC inicial
    uint32_t start = 0;        // primera dirección cargada
    uint32_t end = 0;          // última dirección cargada + 1
    std::map<uint32_t, std::string> symbols;  // solo ELF: dirección -> etiqueta
};

// Carga `path` en memoria. Si el archivo empieza con "\x7FELF" se interpreta
// como ELF de 32 bits para RISC-V; en otro caso, como binario crudo cargado
// en la dirección 0x00000000.
LoadedProgram load_program(const std::string& path, Memory& mem);
