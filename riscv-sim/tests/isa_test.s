# isa_test.s (generado con un script; prueba cada instrucción)
# Prueba cada instrucción del anexo A (y la extensión M). exit2(0) = todo OK;
# exit2(k) = falló la prueba k.
_start:
    li sp, 0x8000
    # prueba 1
    li s0, 1
    li t0, 5
    li t1, -3
    add t2, t0, t1
    li t6, 2
    bne t2, t6, fail
    # prueba 2
    li s0, 2
    li t0, 2147483647
    li t1, 1
    add t2, t0, t1
    li t6, 2147483648
    bne t2, t6, fail
    # prueba 3
    li s0, 3
    li t0, 5
    li t1, 7
    sub t2, t0, t1
    li t6, -2
    bne t2, t6, fail
    # prueba 4
    li s0, 4
    li t0, 0
    li t1, 1
    sub t2, t0, t1
    li t6, 4294967295
    bne t2, t6, fail
    # prueba 5
    li s0, 5
    li t0, 1
    li t1, 31
    sll t2, t0, t1
    li t6, 2147483648
    bne t2, t6, fail
    # prueba 6
    li s0, 6
    li t0, 1
    li t1, 35
    sll t2, t0, t1
    li t6, 8
    bne t2, t6, fail
    # prueba 7
    li s0, 7
    li t0, -1
    li t1, 1
    slt t2, t0, t1
    li t6, 1
    bne t2, t6, fail
    # prueba 8
    li s0, 8
    li t0, 1
    li t1, -1
    slt t2, t0, t1
    li t6, 0
    bne t2, t6, fail
    # prueba 9
    li s0, 9
    li t0, -1
    li t1, 1
    sltu t2, t0, t1
    li t6, 0
    bne t2, t6, fail
    # prueba 10
    li s0, 10
    li t0, 1
    li t1, -1
    sltu t2, t0, t1
    li t6, 1
    bne t2, t6, fail
    # prueba 11
    li s0, 11
    li t0, 61680
    li t1, 4080
    xor t2, t0, t1
    li t6, 65280
    bne t2, t6, fail
    # prueba 12
    li s0, 12
    li t0, 2147483648
    li t1, 4
    srl t2, t0, t1
    li t6, 134217728
    bne t2, t6, fail
    # prueba 13
    li s0, 13
    li t0, 2147483648
    li t1, 4
    sra t2, t0, t1
    li t6, 4160749568
    bne t2, t6, fail
    # prueba 14
    li s0, 14
    li t0, 240
    li t1, 15
    or t2, t0, t1
    li t6, 255
    bne t2, t6, fail
    # prueba 15
    li s0, 15
    li t0, 61680
    li t1, 65280
    and t2, t0, t1
    li t6, 61440
    bne t2, t6, fail
    # prueba 16
    li s0, 16
    li t0, 0
    addi t2, t0, -2048
    li t6, -2048
    bne t2, t6, fail
    # prueba 17
    li s0, 17
    li t0, 10
    addi t2, t0, 2047
    li t6, 2057
    bne t2, t6, fail
    # prueba 18
    li s0, 18
    li t0, -5
    slti t2, t0, -4
    li t6, 1
    bne t2, t6, fail
    # prueba 19
    li s0, 19
    li t0, 3
    slti t2, t0, -4
    li t6, 0
    bne t2, t6, fail
    # prueba 20
    li s0, 20
    li t0, 5
    sltiu t2, t0, -1
    li t6, 1
    bne t2, t6, fail
    # prueba 21
    li s0, 21
    li t0, -1
    sltiu t2, t0, 5
    li t6, 0
    bne t2, t6, fail
    # prueba 22
    li s0, 22
    li t0, 255
    xori t2, t0, -1
    li t6, 4294967040
    bne t2, t6, fail
    # prueba 23
    li s0, 23
    li t0, 256
    ori t2, t0, 15
    li t6, 271
    bne t2, t6, fail
    # prueba 24
    li s0, 24
    li t0, 4660
    andi t2, t0, -16
    li t6, 4656
    bne t2, t6, fail
    # prueba 25
    li s0, 25
    li t0, 3
    slli t2, t0, 30
    li t6, 3221225472
    bne t2, t6, fail
    # prueba 26
    li s0, 26
    li t0, -1
    srli t2, t0, 28
    li t6, 15
    bne t2, t6, fail
    # prueba 27
    li s0, 27
    li t0, -64
    srai t2, t0, 3
    li t6, -8
    bne t2, t6, fail
    # prueba 28
    li s0, 28
    li t0, 1073741824
    srai t2, t0, 30
    li t6, 1
    bne t2, t6, fail
    # prueba 29
    li s0, 29
    lui t2, 0x12345
    li t6, 305418240
    bne t2, t6, fail
    # prueba 30
    li s0, 30
    lui t2, 0xFFFFF
    li t6, 4294963200
    bne t2, t6, fail
    # prueba 31
    li s0, 31
    jal t1, auipc_here
    auipc_here:
    auipc t2, 0
    sub t2, t2, t1
    li t6, 0
    bne t2, t6, fail
    # prueba 32
    li s0, 32
    jal t1, auipc_here2
    auipc_here2:
    auipc t2, 1
    sub t2, t2, t1
    li t6, 4096
    bne t2, t6, fail
    # prueba 33
    li s0, 33
    li t3, 0x2000
    li t0, 0x11223344
    sw t0, 0(t3)
    lw t2, 0(t3)
    li t6, 287454020
    bne t2, t6, fail
    # prueba 34
    li s0, 34
    li t3, 0x2000
    li t0, 0x11223344
    sw t0, 0(t3)
    lbu t2, 0(t3)
    li t6, 68
    bne t2, t6, fail
    # prueba 35
    li s0, 35
    li t3, 0x2000
    li t0, 0x11223344
    sw t0, 0(t3)
    lbu t2, 3(t3)
    li t6, 17
    bne t2, t6, fail
    # prueba 36
    li s0, 36
    li t3, 0x2000
    li t0, 0x11223344
    sw t0, 0(t3)
    lhu t2, 2(t3)
    li t6, 4386
    bne t2, t6, fail
    # prueba 37
    li s0, 37
    li t3, 0x2000
    li t0, 0x80
    sb t0, 4(t3)
    lb t2, 4(t3)
    li t6, -128
    bne t2, t6, fail
    # prueba 38
    li s0, 38
    li t3, 0x2000
    li t0, 0x80
    sb t0, 4(t3)
    lbu t2, 4(t3)
    li t6, 128
    bne t2, t6, fail
    # prueba 39
    li s0, 39
    li t3, 0x2000
    li t0, 0x18001
    sh t0, 8(t3)
    lh t2, 8(t3)
    li t6, -32767
    bne t2, t6, fail
    # prueba 40
    li s0, 40
    li t3, 0x2000
    li t0, 0x18001
    sh t0, 8(t3)
    lhu t2, 8(t3)
    li t6, 32769
    bne t2, t6, fail
    # prueba 41
    li s0, 41
    li t3, 0x2000
    li t0, 0xAB
    sb t0, 1(t3)
    lw t2, 0(t3)
    li t6, 287484740
    bne t2, t6, fail
    # prueba 42
    li s0, 42
    li t3, 0x2010
    li t0, 77
    sw t0, -4(t3)
    lw t2, 12(t3)
    lw t2, -4(t3)
    li t6, 77
    bne t2, t6, fail
    # prueba 43: beq 3,3 -> tomado
    li s0, 43
    li t0, 3
    li t1, 3
    li t2, 0
    beq t0, t1, br_43
    li t2, 1
br_43:
    li t6, 0
    bne t2, t6, fail
    # prueba 44: beq 3,4 -> no tomado
    li s0, 44
    li t0, 3
    li t1, 4
    li t2, 0
    beq t0, t1, br_44
    li t2, 1
br_44:
    li t6, 1
    bne t2, t6, fail
    # prueba 45: bne 3,4 -> tomado
    li s0, 45
    li t0, 3
    li t1, 4
    li t2, 0
    bne t0, t1, br_45
    li t2, 1
br_45:
    li t6, 0
    bne t2, t6, fail
    # prueba 46: bne 3,3 -> no tomado
    li s0, 46
    li t0, 3
    li t1, 3
    li t2, 0
    bne t0, t1, br_46
    li t2, 1
br_46:
    li t6, 1
    bne t2, t6, fail
    # prueba 47: blt -1,1 -> tomado
    li s0, 47
    li t0, -1
    li t1, 1
    li t2, 0
    blt t0, t1, br_47
    li t2, 1
br_47:
    li t6, 0
    bne t2, t6, fail
    # prueba 48: blt 1,-1 -> no tomado
    li s0, 48
    li t0, 1
    li t1, -1
    li t2, 0
    blt t0, t1, br_48
    li t2, 1
br_48:
    li t6, 1
    bne t2, t6, fail
    # prueba 49: blt 2,2 -> no tomado
    li s0, 49
    li t0, 2
    li t1, 2
    li t2, 0
    blt t0, t1, br_49
    li t2, 1
br_49:
    li t6, 1
    bne t2, t6, fail
    # prueba 50: bge 1,-1 -> tomado
    li s0, 50
    li t0, 1
    li t1, -1
    li t2, 0
    bge t0, t1, br_50
    li t2, 1
br_50:
    li t6, 0
    bne t2, t6, fail
    # prueba 51: bge 2,2 -> tomado
    li s0, 51
    li t0, 2
    li t1, 2
    li t2, 0
    bge t0, t1, br_51
    li t2, 1
br_51:
    li t6, 0
    bne t2, t6, fail
    # prueba 52: bge -1,1 -> no tomado
    li s0, 52
    li t0, -1
    li t1, 1
    li t2, 0
    bge t0, t1, br_52
    li t2, 1
br_52:
    li t6, 1
    bne t2, t6, fail
    # prueba 53: bltu 1,-1 -> tomado
    li s0, 53
    li t0, 1
    li t1, -1
    li t2, 0
    bltu t0, t1, br_53
    li t2, 1
br_53:
    li t6, 0
    bne t2, t6, fail
    # prueba 54: bltu -1,1 -> no tomado
    li s0, 54
    li t0, -1
    li t1, 1
    li t2, 0
    bltu t0, t1, br_54
    li t2, 1
br_54:
    li t6, 1
    bne t2, t6, fail
    # prueba 55: bgeu -1,1 -> tomado
    li s0, 55
    li t0, -1
    li t1, 1
    li t2, 0
    bgeu t0, t1, br_55
    li t2, 1
br_55:
    li t6, 0
    bne t2, t6, fail
    # prueba 56: bgeu 1,-1 -> no tomado
    li s0, 56
    li t0, 1
    li t1, -1
    li t2, 0
    bgeu t0, t1, br_56
    li t2, 1
br_56:
    li t6, 1
    bne t2, t6, fail
    # prueba 57: bgeu 5,5 -> tomado
    li s0, 57
    li t0, 5
    li t1, 5
    li t2, 0
    bgeu t0, t1, br_57
    li t2, 1
br_57:
    li t6, 0
    bne t2, t6, fail
    # prueba 58
    li s0, 58
    li t2, 0
    li t0, 5
    back_loop:
    addi t2, t2, 3
    addi t0, t0, -1
    bnez t0, back_loop
    li t6, 15
    bne t2, t6, fail
    # prueba 59
    li s0, 59
    jal t1, jal_tgt
    li t2, 99
    jal_tgt:
    li t2, 7
    li t6, 7
    bne t2, t6, fail
    # prueba 60
    li s0, 60
    jal ra, jal_tgt2
    jal_ret:
    j jal_after
    jal_tgt2:
    la t2, jal_ret
    sub t2, t2, ra
    jr ra
    jal_after:
    li t6, 0
    bne t2, t6, fail
    # prueba 61
    li s0, 61
    la t0, jalr_tgt
    addi t0, t0, 1
    jalr t1, 0(t0)
    li t2, 99
    jalr_tgt:
    li t2, 5
    li t6, 5
    bne t2, t6, fail
    # prueba 62
    li s0, 62
    la t0, jalr_tgt2
    jalr t1, -4(t0)
    jalr_back:
    j jalr_end
    nop
    jalr_tgt2:
    la t2, jalr_back
    sub t2, t2, t1
    j jalr_end2
    jalr_end:
    li t2, 99
    jalr_end2:
    li t6, 0
    bne t2, t6, fail
    # prueba 63
    li s0, 63
    addi zero, zero, 5
    mv t2, zero
    li t6, 0
    bne t2, t6, fail
    # prueba 64
    li s0, 64
    li t0, -7
    li t1, 6
    mul t2, t0, t1
    li t6, -42
    bne t2, t6, fail
    # prueba 65
    li s0, 65
    li t0, -1
    li t1, -1
    mulh t2, t0, t1
    li t6, 0
    bne t2, t6, fail
    # prueba 66
    li s0, 66
    li t0, -1
    li t1, -1
    mulhu t2, t0, t1
    li t6, 4294967294
    bne t2, t6, fail
    # prueba 67
    li s0, 67
    li t0, -1
    li t1, -1
    mulhsu t2, t0, t1
    li t6, -1
    bne t2, t6, fail
    # prueba 68
    li s0, 68
    li t0, -7
    li t1, 2
    div t2, t0, t1
    li t6, -3
    bne t2, t6, fail
    # prueba 69
    li s0, 69
    li t0, 7
    li t1, 0
    div t2, t0, t1
    li t6, -1
    bne t2, t6, fail
    # prueba 70
    li s0, 70
    li t0, 2147483648
    li t1, -1
    div t2, t0, t1
    li t6, 2147483648
    bne t2, t6, fail
    # prueba 71
    li s0, 71
    li t0, -1
    li t1, 2
    divu t2, t0, t1
    li t6, 2147483647
    bne t2, t6, fail
    # prueba 72
    li s0, 72
    li t0, -7
    li t1, 2
    rem t2, t0, t1
    li t6, -1
    bne t2, t6, fail
    # prueba 73
    li s0, 73
    li t0, 7
    li t1, 0
    rem t2, t0, t1
    li t6, 7
    bne t2, t6, fail
    # prueba 74
    li s0, 74
    li t0, 7
    li t1, 3
    remu t2, t0, t1
    li t6, 1
    bne t2, t6, fail
    li a0, 0
    li a7, 17
    ecall
fail:
    mv a0, s0
    li a7, 17
    ecall
