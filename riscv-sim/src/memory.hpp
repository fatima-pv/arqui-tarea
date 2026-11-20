#pragma once
// Memoria plana, byte-addressable, de 32 bits (4 GiB de address space).
// Se implementa de forma "sparse": solo se reservan páginas de 4 KiB cuando
// se escriben. Leer una dirección nunca escrita devuelve 0.
// Todos los accesos multi-byte son little-endian, como exige RISC-V.

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>

class Memory {
public:
    static constexpr uint32_t PAGE_BITS = 12;
    static constexpr uint32_t PAGE_SIZE = 1u << PAGE_BITS;

    uint8_t read8(uint32_t addr) const {
        auto it = pages_.find(addr >> PAGE_BITS);
        if (it == pages_.end()) return 0;
        return (*it->second)[addr & (PAGE_SIZE - 1)];
    }

    void write8(uint32_t addr, uint8_t value) {
        auto& page = pages_[addr >> PAGE_BITS];
        if (!page) {
            page = std::make_unique<Page>();
            page->fill(0);
        }
        (*page)[addr & (PAGE_SIZE - 1)] = value;
    }

    uint16_t read16(uint32_t addr) const {
        return static_cast<uint16_t>(read8(addr) | (read8(addr + 1) << 8));
    }

    uint32_t read32(uint32_t addr) const {
        return static_cast<uint32_t>(read8(addr)) |
               (static_cast<uint32_t>(read8(addr + 1)) << 8) |
               (static_cast<uint32_t>(read8(addr + 2)) << 16) |
               (static_cast<uint32_t>(read8(addr + 3)) << 24);
    }

    void write16(uint32_t addr, uint16_t value) {
        write8(addr, static_cast<uint8_t>(value));
        write8(addr + 1, static_cast<uint8_t>(value >> 8));
    }

    void write32(uint32_t addr, uint32_t value) {
        write8(addr, static_cast<uint8_t>(value));
        write8(addr + 1, static_cast<uint8_t>(value >> 8));
        write8(addr + 2, static_cast<uint8_t>(value >> 16));
        write8(addr + 3, static_cast<uint8_t>(value >> 24));
    }

    void clear() { pages_.clear(); }

    size_t pages_in_use() const { return pages_.size(); }

private:
    using Page = std::array<uint8_t, PAGE_SIZE>;
    std::unordered_map<uint32_t, std::unique_ptr<Page>> pages_;
};
