# fibonacci_io.s
# Ejemplo de ecall (estilo SPIM/CPUlator): pide n por teclado e imprime los
# primeros n números de Fibonacci. Termina con exit2 (a7 = 17) y código 0.
#   a7 = 4  print_string   a7 = 1  print_int   a7 = 11 print_char
#   a7 = 5  read_int       a7 = 17 exit2(a0)

_start:
    li   sp, 0x8000
    la   a0, prompt
    li   a7, 4
    ecall                   # print_string(prompt)
    li   a7, 5
    ecall                   # a0 = read_int()
    mv   s0, a0             # s0 = n

    la   a0, header
    li   a7, 4
    ecall

    li   s1, 0              # a = 0
    li   s2, 1              # b = 1
loop:
    beqz s0, finish
    mv   a0, s1
    li   a7, 1
    ecall                   # print_int(a)
    li   a0, 32             # ' '
    li   a7, 11
    ecall                   # print_char(' ')
    add  t0, s1, s2         # (a, b) = (b, a + b)
    mv   s1, s2
    mv   s2, t0
    addi s0, s0, -1
    j    loop

finish:
    li   a0, 10             # '\n'
    li   a7, 11
    ecall
    li   a0, 0
    li   a7, 17
    ecall                   # exit2(0)

prompt: .asciz "Cuantos numeros de Fibonacci? "
header: .asciz "Fibonacci: "
