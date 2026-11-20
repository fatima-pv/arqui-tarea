# quicksort.s
# Coloca el arreglo {6, 4, 3, 2, 1, 8, 9} en 0x1000 y lo ordena in-place con
# quicksort recursivo (partición de Lomuto, pivote = arr[r-1]).
# Resultado esperado en 0x1000: {1, 2, 3, 4, 6, 8, 9}.
# Nota: no inicializa sp (vale 0 como en CPUlator), así que la pila crece
# hacia 0xFFFFFFxx; la memoria del simulador cubre los 32 bits completos.

_start:
    li   t0, 0x1000
    li   t1, 6
    sw   t1, 0(t0)
    li   t1, 4
    sw   t1, 4(t0)
    li   t1, 3
    sw   t1, 8(t0)
    li   t1, 2
    sw   t1, 12(t0)
    li   t1, 1
    sw   t1, 16(t0)
    li   t1, 8
    sw   t1, 20(t0)
    li   t1, 9
    sw   t1, 24(t0)

    li   a0, 0x1000         # arr
    li   a1, 0              # l
    li   a2, 7              # r (exclusivo)
    call qsort
_end:
    j    _end

# qsort(a0 = arr, a1 = l, a2 = r): ordena arr[l .. r-1]
qsort:
    addi t0, a1, 1
    bge  t0, a2, qsort_ret  # si l + 1 >= r, ya está ordenado
    addi sp, sp, -20
    sw   ra, 16(sp)
    sw   s0, 12(sp)
    sw   s1, 8(sp)
    sw   s2, 4(sp)
    sw   s3, 0(sp)
    mv   s0, a0
    mv   s1, a1
    mv   s2, a2
    call partition          # a0 = p
    mv   s3, a0
    mv   a0, s0
    mv   a1, s1
    mv   a2, s3
    call qsort              # qsort(arr, l, p)
    mv   a0, s0
    addi a1, s3, 1
    mv   a2, s2
    call qsort              # qsort(arr, p + 1, r)
    lw   ra, 16(sp)
    lw   s0, 12(sp)
    lw   s1, 8(sp)
    lw   s2, 4(sp)
    lw   s3, 0(sp)
    addi sp, sp, 20
qsort_ret:
    ret

# partition(a0 = arr, a1 = l, a2 = r) -> a0 = posición final del pivote
partition:
    addi t0, a2, -1
    slli t0, t0, 2
    add  t0, a0, t0         # t0 = &arr[r-1]
    lw   t1, 0(t0)          # t1 = pivote
    mv   t2, a1             # i = l
    mv   t3, a1             # j = l
    addi t4, a2, -1         # t4 = r - 1
part_loop:
    bge  t3, t4, part_done
    slli t5, t3, 2
    add  t5, a0, t5         # &arr[j]
    lw   t6, 0(t5)
    bge  t6, t1, part_next  # si arr[j] >= pivote, no intercambiar
    slli a3, t2, 2
    add  a3, a0, a3         # &arr[i]
    lw   a4, 0(a3)
    sw   t6, 0(a3)          # swap(arr[i], arr[j])
    sw   a4, 0(t5)
    addi t2, t2, 1          # i++
part_next:
    addi t3, t3, 1          # j++
    j    part_loop
part_done:
    slli a3, t2, 2
    add  a3, a0, a3
    lw   a4, 0(a3)
    sw   t1, 0(a3)          # swap(arr[i], arr[r-1])
    sw   a4, 0(t0)
    mv   a0, t2
    ret
