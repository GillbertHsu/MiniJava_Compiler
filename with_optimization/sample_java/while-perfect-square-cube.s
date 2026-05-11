.section .data
.Lfmt_int: .asciz "%ld"
.Lfmt_str: .asciz "%s"
.Lfmt_nl: .asciz "\n"

.section .text
.globl main

__str_concat:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    subq $8, %rsp
    movq %rdi, %r12
    movq %rsi, %r13
    movq %r12, %rdi
    call strlen
    movq %rax, %rbx
    movq %r13, %rdi
    call strlen
    addq %rbx, %rax
    addq $1, %rax
    movq %rax, %rdi
    call malloc
    movq %rax, %rbx
    movq %rbx, %rdi
    movq %r12, %rsi
    call strcpy
    movq %rbx, %rdi
    movq %r13, %rsi
    call strcat
    movq %rbx, %rax
    addq $8, %rsp
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

main:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $112, %rsp
    movq %rdi, %rax
    movq %rsi, %r11
    movq $8, %r10
    movq %r11, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r11
    movq $0, %r10
    movq %r11, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, -72(%rbp)
    movq $0, %rsi
    movq $1, %rdi
    movq $0, %rax
    movq %rax, -64(%rbp)
    movq %rdi, %r15
    movq -64(%rbp), %rax
    movq %rax, %r14
    movq %rsi, %r13
    jmp .Lmain_while_header_1
.Lmain_while_header_1:
    movq %r13, %rax
    movq -72(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_2
    jmp .Lmain_while_exit_3
.Lmain_while_body_2:
    movq $1, %rbx
    movq $0, %r12
    movq %rbx, %r9
    movq %r12, %r8
    jmp .Lmain_while_header_4
.Lmain_while_exit_3:
    movq %r14, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r10
    movq %r10, %rax
    addq $112, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.Lmain_while_header_4:
    movq %r9, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r15, %rcx
    cmpq %rcx, %rax
    setle %al
    movzbq %al, %rax
    movq %rax, %r11
    movq %r8, %rax
    xorq $1, %rax
    movq %rax, %r10
    movq %r11, %rax
    movq %r10, %rcx
    andq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_5
    jmp .Lmain_while_exit_6
.Lmain_while_body_5:
    movq %r9, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r15, %rcx
    cmpq %rcx, %rax
    sete %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_if_then_7
    jmp .Lmain_if_else_8
.Lmain_while_exit_6:
    movq %r8, %rax
    testq %rax, %rax
    jne .Lmain_if_then_10
    jmp .Lmain_if_else_11
.Lmain_if_then_7:
    movq $1, %rax
    movq %rax, -120(%rbp)
    movq -120(%rbp), %rax
    movq %rax, -104(%rbp)
    jmp .Lmain_if_merge_9
.Lmain_if_else_8:
    movq $0, %rax
    movq %rax, -128(%rbp)
    movq -128(%rbp), %rax
    movq %rax, -104(%rbp)
    jmp .Lmain_if_merge_9
.Lmain_if_merge_9:
    movq $1, %r10
    movq %r9, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -88(%rbp)
    movq -88(%rbp), %rax
    movq %rax, %r9
    movq -104(%rbp), %rax
    movq %rax, %r8
    jmp .Lmain_while_header_4
.Lmain_if_then_10:
    movq $1, %rbx
    movq $0, %r12
    movq %rbx, %r9
    movq %r12, %r8
    jmp .Lmain_while_header_13
.Lmain_if_else_11:
    movq %r14, %r12
    jmp .Lmain_if_merge_12
.Lmain_if_merge_12:
    movq $1, %r10
    movq %r15, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %rbx
    movq %rbx, %r15
    movq %r12, %r14
    jmp .Lmain_while_header_1
.Lmain_while_header_13:
    movq %r9, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r15, %rcx
    cmpq %rcx, %rax
    setle %al
    movzbq %al, %rax
    movq %rax, %r11
    movq %r8, %rax
    xorq $1, %rax
    movq %rax, %r10
    movq %r11, %rax
    movq %r10, %rcx
    andq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_14
    jmp .Lmain_while_exit_15
.Lmain_while_body_14:
    movq %r9, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r9, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r15, %rcx
    cmpq %rcx, %rax
    sete %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_if_then_16
    jmp .Lmain_if_else_17
.Lmain_while_exit_15:
    movq %r8, %rax
    testq %rax, %rax
    jne .Lmain_if_then_19
    jmp .Lmain_if_else_20
.Lmain_if_then_16:
    movq $1, %rax
    movq %rax, -144(%rbp)
    movq -144(%rbp), %rax
    movq %rax, -96(%rbp)
    jmp .Lmain_if_merge_18
.Lmain_if_else_17:
    movq $0, %rax
    movq %rax, -136(%rbp)
    movq -136(%rbp), %rax
    movq %rax, -96(%rbp)
    jmp .Lmain_if_merge_18
.Lmain_if_merge_18:
    movq $1, %r10
    movq %r9, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -80(%rbp)
    movq -80(%rbp), %rax
    movq %rax, %r9
    movq -96(%rbp), %rax
    movq %rax, %r8
    jmp .Lmain_while_header_13
.Lmain_if_then_19:
    movq $1, %r10
    movq %r13, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -112(%rbp)
    movq %r15, %rax
    movq %rax, -56(%rbp)
    movq -112(%rbp), %rax
    movq %rax, -48(%rbp)
    jmp .Lmain_if_merge_21
.Lmain_if_else_20:
    movq %r14, %rax
    movq %rax, -56(%rbp)
    movq %r13, %rax
    movq %rax, -48(%rbp)
    jmp .Lmain_if_merge_21
.Lmain_if_merge_21:
    movq -56(%rbp), %rax
    movq %rax, %r12
    movq -48(%rbp), %rax
    movq %rax, %r13
    jmp .Lmain_if_merge_12

