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
    subq $8, %rsp
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
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $3, %r10
    movq %r10, %rdi
    addq $1, %rdi
    shlq $3, %rdi
    pushq %rdi
    subq $8, %rsp
    movq 8(%rsp), %rdi
    call malloc
    addq $8, %rsp
    popq %rcx
    shrq $3, %rcx
    decq %rcx
    movq %rcx, (%rax)
    addq $8, %rax
    movq %rax, %r12
    movq $1, %r10
    movq $0, %r11
    movq %r12, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $2, %r10
    movq $1, %r11
    movq %r12, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $4, %r10
    movq $2, %r11
    movq %r12, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $3, %r10
    movq %r10, %rdi
    addq $1, %rdi
    shlq $3, %rdi
    pushq %rdi
    subq $8, %rsp
    movq 8(%rsp), %rdi
    call malloc
    addq $8, %rsp
    popq %rcx
    shrq $3, %rcx
    decq %rcx
    movq %rcx, (%rax)
    addq $8, %rax
    movq %rax, %rbx
    movq $5, %r10
    movq $0, %r11
    movq %rbx, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $3, %r10
    movq $1, %r11
    movq %rbx, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r10
    movq $2, %r11
    movq %rbx, %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r10
    movq $2, %r11
    movq %r10, %r14
    movq %r11, %r13
    jmp .Lmain_while_header_1
.Lmain_while_header_1:
    movq $0, %r8
    movq %r13, %rax
    movq %r8, %rcx
    cmpq %rcx, %rax
    setge %al
    movzbq %al, %rax
    movq %rax, %r8
    movq %r8, %rax
    testq %rax, %rax
    jne .Lmain_while_body_2
    jmp .Lmain_while_exit_3
.Lmain_while_body_2:
    movq %r12, %rax
    movq %r13, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r9
    movq %rbx, %rax
    movq %r13, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq %r9, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r8
    movq %r14, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r14
    movq $1, %r8
    movq %r13, %rax
    movq %r8, %rcx
    subq %rcx, %rax
    movq %rax, %r13
    jmp .Lmain_while_header_1
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
    addq $8, %rsp
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

