#include "loader.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

uint16_t rd16(const std::vector<uint8_t>& b, size_t o) {
    return static_cast<uint16_t>(b[o] | (b[o + 1] << 8));
}
uint32_t rd32(const std::vector<uint8_t>& b, size_t o) {
    return b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (static_cast<uint32_t>(b[o + 3]) << 24);
}

LoadedProgram load_raw(const std::vector<uint8_t>& data, Memory& mem) {
    LoadedProgram p;
    p.format = "binario crudo";
    for (size_t i = 0; i < data.size(); ++i) mem.write8(static_cast<uint32_t>(i), data[i]);
    p.ok = true;
    p.entry = 0;
    p.start = 0;
    p.end = static_cast<uint32_t>(data.size());
    return p;
}

LoadedProgram load_elf(const std::vector<uint8_t>& b, Memory& mem) {
    LoadedProgram p;
    p.format = "ELF32";
    auto fail = [&](const std::string& e) {
        p.ok = false;
        p.error = e;
        return p;
    };
    if (b.size() < 52) return fail("ELF demasiado pequeño");
    if (b[4] != 1) return fail("solo se soportan ELF de 32 bits (ELFCLASS32)");
    if (b[5] != 1) return fail("el ELF no es little-endian");
    if (rd16(b, 18) != 243) return fail("el ELF no es para RISC-V (e_machine != 243)");

    const uint32_t entry = rd32(b, 24);
    const uint32_t phoff = rd32(b, 28);
    const uint32_t shoff = rd32(b, 32);
    const uint16_t phentsize = rd16(b, 42), phnum = rd16(b, 44);
    const uint16_t shentsize = rd16(b, 46), shnum = rd16(b, 48);

    bool any = false;
    p.start = 0xFFFFFFFF;
    p.end = 0;
    for (uint32_t i = 0; i < phnum; ++i) {
        size_t ph = phoff + static_cast<size_t>(i) * phentsize;
        if (ph + 32 > b.size()) return fail("program header fuera del archivo");
        if (rd32(b, ph) != 1) continue;  // solo PT_LOAD
        uint32_t offset = rd32(b, ph + 4), vaddr = rd32(b, ph + 8);
        uint32_t filesz = rd32(b, ph + 16), memsz = rd32(b, ph + 20);
        if (static_cast<size_t>(offset) + filesz > b.size()) return fail("segmento fuera del archivo");
        for (uint32_t k = 0; k < memsz; ++k)
            mem.write8(vaddr + k, k < filesz ? b[offset + k] : 0);  // .bss = 0
        any = true;
        if (vaddr < p.start) p.start = vaddr;
        if (vaddr + memsz > p.end) p.end = vaddr + memsz;
    }
    if (!any) return fail("el ELF no tiene segmentos PT_LOAD");

    // Tabla de símbolos (opcional): sirve para mostrar etiquetas en el desensamblado.
    if (shoff && shentsize >= 40 && shoff + static_cast<size_t>(shnum) * shentsize <= b.size()) {
        for (uint32_t i = 0; i < shnum; ++i) {
            size_t sh = shoff + static_cast<size_t>(i) * shentsize;
            if (rd32(b, sh + 4) != 2) continue;  // SHT_SYMTAB
            uint32_t off = rd32(b, sh + 16), size = rd32(b, sh + 20);
            uint32_t link = rd32(b, sh + 24), entsize = rd32(b, sh + 36);
            if (link >= shnum || entsize < 16) continue;
            size_t strsh = shoff + static_cast<size_t>(link) * shentsize;
            uint32_t stroff = rd32(b, strsh + 16), strsize = rd32(b, strsh + 20);
            if (static_cast<size_t>(stroff) + strsize > b.size()) continue;
            for (uint32_t s = 0; s + entsize <= size; s += entsize) {
                size_t sym = off + s;
                if (sym + 16 > b.size()) break;
                uint32_t name = rd32(b, sym), value = rd32(b, sym + 4);
                uint8_t type = b[sym + 12] & 0xF;
                if (name == 0 || name >= strsize || type > 2) continue;  // NOTYPE/OBJECT/FUNC
                const char* str = reinterpret_cast<const char*>(&b[stroff + name]);
                std::string n(str, strnlen(str, strsize - name));
                if (!n.empty() && n[0] != '$' && n[0] != '.') p.symbols.emplace(value, n);
            }
        }
    }

    p.ok = true;
    p.entry = entry;
    return p;
}

}  // namespace

LoadedProgram load_program(const std::string& path, Memory& mem) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        LoadedProgram p;
        p.error = "no se pudo abrir \"" + path + "\"";
        return p;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    mem.clear();
    if (data.size() >= 4 && std::memcmp(data.data(), "\x7F" "ELF", 4) == 0) return load_elf(data, mem);
    return load_raw(data, mem);
}
