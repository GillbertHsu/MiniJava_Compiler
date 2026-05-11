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
    movq $1, %rbx
    movq %rbx, %r12
    jmp .Lmain_while_header_1
.Lmain_while_header_1:
    movq $10, %r10
    movq %r12, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_2
    jmp .Lmain_while_exit_3
.Lmain_while_body_2:
    movq %r12, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r12
    jmp .Lmain_while_header_1
.Lmain_while_exit_3:
    movq $0, %r10
    movq %r10, %rax
    addq $8, %rsp
    popq %r12
    popq %rbx
    popq %rbp
    ret

