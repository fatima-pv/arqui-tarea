#!/usr/bin/env bash
# Pruebas automáticas del simulador. Uso: make test  (o ./tests/run_tests.sh)
set -u
cd "$(dirname "$0")/.."
SIM=./build/rvsim
pass=0
fail=0

check() {  # check <nombre> <salida> <texto esperado>
    if grep -qF -- "$3" <<<"$2"; then
        echo "  OK    $1"
        pass=$((pass + 1))
    else
        echo "  FALLA $1  (se esperaba: $3)"
        echo "$2" | sed 's/^/        /'
        fail=$((fail + 1))
    fi
}

echo "Pruebas del simulador RISC-V"

out=$($SIM tests/isa_test.bin --run)
check "isa_test: todas las instrucciones RV32I + M" "$out" "exit(0)"

out=$(printf 'run\nmem 100 103\nexit\n' | $SIM examples/riscvtest.bin)
check "riscvtest: escribe 25 en 0x64" "$out" "Memoria (0x00000064-0x00000067): 19 00 00 00"

out=$(printf 'run\nmem 0x1000 0x101B\nexit\n' | $SIM examples/quicksort.bin)
check "quicksort: {1,2,3,4,6,8,9} en 0x1000" "$out" \
    "01 00 00 00 02 00 00 00  03 00 00 00 04 00 00 00"
check "quicksort: segunda mitad" "$out" "06 00 00 00 08 00 00 00  09 00 00 00"

out=$(printf 'run\nregs a0\nexit\n' | $SIM examples/symmetric_tree.bin)
check "symmetric_tree: a0 = 1" "$out" "x10 (a0) = 0x00000001"

out=$(echo 10 | $SIM examples/fibonacci_io.bin --run)
check "fibonacci_io: ecall read_int/print_int" "$out" "Fibonacci: 0 1 1 2 3 5 8 13 21 34"

out=$(printf 'run\nmemw 0x1000 1\nexit\n' | $SIM examples/quicksort.elf)
check "carga ELF (entry 0x10000)" "$out" "0x00001000: 0x00000001  1"

out=$(printf 'step\nstep\nstep\npc\nexit\n' | $SIM examples/riscvtest.bin)
check "step x3 -> pc = 0x0000000C" "$out" "pc = 0x0000000C"

out=$(printf 'set pc 0x200\nstep\nexit\n' | $SIM examples/riscvtest.bin)
check "instrucción inválida se reporta" "$out" "Error de ejecución: instrucción inválida 0x00000000"

echo
echo "$pass pruebas OK, $fail fallidas"
[ "$fail" -eq 0 ]
