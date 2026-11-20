# rvsim — Simulador de RISC-V (RV32I)

Simulador del ISA base RISC-V de 32 bits escrito en **C++17**. Carga un programa
ya compilado (binario crudo exportado de CPUlator, o un ELF) y permite ejecutarlo
paso a paso, inspeccionando el PC, los registros y la memoria.

## Compilar

```bash
make              # genera build/rvsim
make test         # corre las pruebas automáticas
```

También se puede usar CMake: `cmake -S . -B build && cmake --build build`.
Solo necesita un compilador de C++17 (g++ o clang++); no tiene dependencias externas.

## Uso

```bash
./build/rvsim programa.bin            # modo interactivo
./build/rvsim programa.bin --run      # ejecuta hasta terminar e imprime los registros
./build/rvsim programa.elf --run -t   # igual, mostrando cada instrucción (trace)
```

Ejemplo de sesión:

```
$ ./build/rvsim examples/riscvtest.bin
"examples/riscvtest.bin" cargado a memoria (binario crudo, 84 bytes en 0x00000000-0x00000053, pc = 0x00000000).
> step
  [0x00000000] 00500113  li sp, 5
      x2  (sp  ): 0x00000000 -> 0x00000005  (5)
> step 2
  [0x00000004] 00c00193  li gp, 12
      x3  (gp  ): 0x00000000 -> 0x0000000C  (12)
  [0x00000008] ff718393  addi t2, gp, -9
      x7  (t2  ): 0x00000000 -> 0x00000003  (3)
> pc
pc = 0x0000000C
> run
Programa terminado: bucle infinito en 0x00000050 (fin del programa) tras 19 instrucciones.
> mem 100 103
Memoria (0x00000064-0x00000067): 19 00 00 00
> exit
See you next time...
Program exited with code 0.
```

### Comandos

| Comando | Descripción |
|---|---|
| `step [n]` (`s`) | Ejecuta 1 (o n) instrucciones; muestra la instrucción desensamblada, los registros que cambiaron y lo escrito en memoria |
| `run [max]` (`r`, `c`) | Ejecuta hasta que el programa termina, llega a un breakpoint o a `max` instrucciones |
| `pc` | Muestra el PC |
| `regs [r ...]` | Muestra los 32 registros, o solo los indicados (`x5`, `a0`, `sp`, `fp`…) |
| `mem <ini> [fin]` | Muestra bytes de memoria en un rango inclusivo (hexdump si es largo) |
| `memw <dir> [n]` | Muestra n palabras de 32 bits en hexadecimal y decimal |
| `disasm [dir] [n]` (`d`) | Desensambla n instrucciones (por defecto desde el PC, marcado con `=>`) |
| `break <dir>` / `delete <dir\|all>` / `breaks` | Manejo de breakpoints |
| `set <reg\|pc> <valor>` / `poke <dir> <valor>` | Modifica un registro, el PC o una palabra de memoria |
| `trace on\|off` | Muestra cada instrucción durante `run` |
| `info` | Datos del programa cargado (formato, rango, entry point, símbolos) |
| `reset` / `load <archivo>` | Recarga el programa / carga otro |
| `help`, `exit` | Ayuda y salida |

Los valores aceptan decimal, hexadecimal (`0x…`), nombres de registro (`mem sp`) y, con ELF,
etiquetas (`break partition`). Una línea vacía repite el último comando.

## Funcionalidades

**Obligatorias**

- Las 37 instrucciones del anexo A (RV32I): loads/stores de byte, half y word (con y sin signo),
  aritméticas y lógicas con registro e inmediato, shifts, `lui`, `auipc`, branches, `jal`, `jalr`.
- Memoria plana *byte-addressable* de 32 bits en formato **little-endian**.
- Carga de binarios crudos en la dirección `0x00000000` (formato "Raw" de CPUlator).
- Ejecución paso a paso e inspección de PC, registros y memoria.

**Adicionales**

- **Desensamblador** con nombres ABI y pseudo-instrucciones (`li`, `mv`, `j`, `ret`, `beqz`…).
- **`ecall`** con las llamadas de SPIM/CPUlator: `print_int` (1), `print_string` (4), `read_int` (5),
  `read_string` (8), `exit` (10), `print_char` (11), `read_char` (12), `exit2` (17),
  `print_hex` (34), `print_bin` (35), `print_unsigned` (36) y `exit` de Linux (93).
- **Carga de archivos ELF32** de RISC-V (segmentos `PT_LOAD`, entry point y tabla de símbolos).
- **Breakpoints**, `ebreak`, modo *trace* y modo no interactivo (`--run`).
- **Extensión M** (`mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu`).

## Diseño

```
src/memory.hpp   Memoria sparse: páginas de 4 KiB creadas al escribir (cubre los 4 GiB)
src/cpu.*        Estado (pc, x0..x31, memoria) y ciclo fetch -> decode -> execute; ecalls
src/disasm.*     Desensamblador y nombres de registros
src/loader.*     Carga de binario crudo y ELF32
src/main.cpp     Intérprete de comandos (REPL)
```

- `Cpu::step()` lee la palabra apuntada por el PC, extrae `opcode`, `rd`, `rs1`, `rs2`, `funct3`,
  `funct7` y el inmediato según el formato (I, S, B, U, J), ejecuta y actualiza el PC.
  `x0` siempre vale 0.
- Como no hay sistema operativo, un programa "termina" cuando salta a sí mismo
  (`j .`, `done: beq x2, x2, done`), con `ecall` de salida o con un error.
- Las instrucciones inválidas, el PC desalineado y las `ecall` desconocidas se reportan
  explícitamente y detienen la ejecución.
- La memoria es sparse, así que un programa que no inicializa `sp` (vale 0, como en CPUlator)
  puede usar la pila en `0xFFFFFFxx` sin problemas.

## Ejemplos y pruebas

| Archivo | Qué hace | Resultado esperado |
|---|---|---|
| `examples/riscvtest.bin` | Test de Harris & Harris | `mem[0x64] = 25` |
| `examples/quicksort.bin` | Ordena `{6,4,3,2,1,8,9}` en 0x1000 | `{1,2,3,4,6,8,9}` |
| `examples/symmetric_tree.bin` | `is_symmetric` sobre `{6,4,4,1,2,2,1}` | `a0 = 1` |
| `examples/fibonacci_io.bin` | Lee n con `ecall` e imprime Fibonacci | texto en consola |
| `examples/quicksort.elf` | Quicksort como ELF con símbolos (entry 0x10000) | igual que quicksort |
| `tests/isa_test.bin` | 74 pruebas autoverificables de cada instrucción | `exit(0)` |

Los `.bin` se pueden regenerar con `make examples`. `tools/rvasm.py` es un mini-ensamblador
**solo para generar los programas de prueba** (no forma parte del simulador); los binarios
también se pueden exportar desde CPUlator (Memory → Save, formato *Raw*, desde 0x0000).
