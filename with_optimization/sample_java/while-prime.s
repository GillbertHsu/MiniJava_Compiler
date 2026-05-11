.section .data
.Lstr0: .asciz "true"
.Lstr1: .asciz "false"
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

sqrt:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    movq %rdi, %r10
    movq $0, %r13
    movq $0, %r11
    movq %r11, %r12
    movq %r13, %rbx
    jmp .Lsqrt_while_header_1
.Lsqrt_while_header_1:
    movq %r12, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setle %al
    movzbq %al, %rax
    movq %rax, %r8
    movq %r8, %rax
    testq %rax, %rax
    jne .Lsqrt_while_body_2
    jmp .Lsqrt_while_exit_3
.Lsqrt_while_body_2:
    movq $2, %r8
    movq %r8, %rax
    movq %rbx, %rcx
    imulq %rcx, %rax
    movq %rax, %r9
    movq $1, %r8
    movq %r9, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r8
    movq %r12, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r9
    movq $1, %r8
    movq %rbx, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r8
    movq %r9, %r12
    movq %r8, %rbx
    jmp .Lsqrt_while_header_1
.Lsqrt_while_exit_3:
    movq $1, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    subq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

isPrime:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $16, %rsp
    movq %rdi, -48(%rbp)
    movq $1, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setle %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisPrime_if_then_1
    jmp .LisPrime_if_else_2
.LisPrime_if_then_1:
    movq $0, %r10
    movq %r10, %rax
    addq $16, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.LisPrime_if_else_2:
    movq $2, %r13
    movq %r13, %r12
    jmp .LisPrime_while_header_4
.LisPrime_if_merge_3:
.LisPrime_while_header_4:
    movq -48(%rbp), %rdi
    call sqrt
    movq %rax, %rbx
    movq %r12, %rax
    movq %rbx, %rcx
    cmpq %rcx, %rax
    setle %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisPrime_while_body_5
    jmp .LisPrime_while_exit_6
.LisPrime_while_body_5:
    movq $0, %r9
    movq %r9, %r8
    movq -48(%rbp), %rax
    movq %rax, %r11
    jmp .LisPrime_while_header_7
.LisPrime_while_exit_6:
    movq $1, %r10
    movq %r10, %rax
    addq $16, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.LisPrime_while_header_7:
    movq %r11, %rax
    movq %r12, %rcx
    cmpq %rcx, %rax
    setge %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisPrime_while_body_8
    jmp .LisPrime_while_exit_9
.LisPrime_while_body_8:
    movq %r11, %rax
    movq %r12, %rcx
    subq %rcx, %rax
    movq %rax, %r14
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r15
    movq %r15, %r8
    movq %r14, %r11
    jmp .LisPrime_while_header_7
.LisPrime_while_exit_9:
    movq $0, %r10
    movq %r11, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    sete %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisPrime_if_then_10
    jmp .LisPrime_if_else_11
.LisPrime_if_then_10:
    movq $0, %r10
    movq %r10, %rax
    addq $16, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.LisPrime_if_else_11:
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r10
    movq %r10, %r11
    jmp .LisPrime_if_merge_12
.LisPrime_if_merge_12:
    movq %r11, %r12
    jmp .LisPrime_while_header_4

main:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
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
    movq %rax, %rbx
    movq %rbx, %rdi
    call isPrime
    movq %rax, %rbx
    movq %rbx, %rax
    testq %rax, %rax
    jne .Lmain_if_then_1
    jmp .Lmain_if_else_2
.Lmain_if_then_1:
    leaq .Lstr0(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    jmp .Lmain_if_merge_3
.Lmain_if_else_2:
    leaq .Lstr1(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    jmp .Lmain_if_merge_3
.Lmain_if_merge_3:
    movq $0, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret

