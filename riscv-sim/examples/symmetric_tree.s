# symmetric_tree.s
# Coloca el árbol binario completo aplanado {6, 4, 4, 1, 2, 2, 1} en 0x1000
# (hijos de i en 2i+1 y 2i+2) y evalúa is_symmetric(arr, n).
# Como el árbol es simétrico, el programa termina con a0 = 1.
#
#            6
#          /   \
#         4     4
#        / \   / \
#       1   2 2   1

_start:
    li   t0, 0x1000
    li   t1, 6
    sw   t1, 0(t0)
    li   t1, 4
    sw   t1, 4(t0)
    sw   t1, 8(t0)
    li   t1, 1
    sw   t1, 12(t0)
    li   t1, 2
    sw   t1, 16(t0)
    sw   t1, 20(t0)
    li   t1, 1
    sw   t1, 24(t0)

    li   a0, 0x1000
    li   a1, 7
    call is_symmetric       # a0 = 1 si es simétrico, 0 si no
_end:
    j    _end

# is_symmetric(a0 = arr, a1 = n)
is_symmetric:
    beqz a1, sym_true       # el árbol vacío es simétrico
    li   a2, 1
    li   a3, 2
    j    solve              # tail call: solve(arr, n, 1, 2)
sym_true:
    li   a0, 1
    ret

# solve(a0 = arr, a1 = n, a2 = l, a3 = r): ¿el subárbol l es espejo del subárbol r?
solve:
    slt  t0, a2, a1         # t0 = l < n
    slt  t1, a3, a1         # t1 = r < n
    or   t2, t0, t1
    beqz t2, ret_true       # ambos nulos -> true
    and  t2, t0, t1
    beqz t2, ret_false      # solo uno nulo -> false
    slli t0, a2, 2
    add  t0, a0, t0
    lw   t0, 0(t0)          # arr[l]
    slli t1, a3, 2
    add  t1, a0, t1
    lw   t1, 0(t1)          # arr[r]
    bne  t0, t1, ret_false

    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   a2, 8(sp)
    sw   a3, 4(sp)
    sw   a0, 0(sp)
    # solve(arr, n, 2l+1, 2r+2)
    slli a2, a2, 1
    addi a2, a2, 1
    slli a3, a3, 1
    addi a3, a3, 2
    call solve
    beqz a0, solve_out      # cortocircuito: si da false, devolver false
    lw   a0, 0(sp)
    lw   a2, 8(sp)
    lw   a3, 4(sp)
    # solve(arr, n, 2l+2, 2r+1)
    slli a2, a2, 1
    addi a2, a2, 2
    slli a3, a3, 1
    addi a3, a3, 1
    call solve
solve_out:
    lw   ra, 12(sp)
    addi sp, sp, 16
    ret
ret_true:
    li   a0, 1
    ret
ret_false:
    li   a0, 0
    ret
