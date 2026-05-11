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
    movq %rdi, %rax
    movq %rsi, %r11
    movq $8, %r10
    movq %r11, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %rbx
    movq $0, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, %r15
    movq $1, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, %r14
    movq $2, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, %rbx
    movq $0, %r11
    movq %rbx, %r10
    movq %r11, %r12
    jmp .Lmain_while_header_1
.Lmain_while_header_1:
    movq $0, %r8
    movq %r10, %rax
    movq %r8, %rcx
    cmpq %rcx, %rax
    setg %al
    movzbq %al, %rax
    movq %rax, %r8
    movq %r8, %rax
    testq %rax, %rax
    jne .Lmain_while_body_2
    jmp .Lmain_while_exit_3
.Lmain_while_body_2:
    movq %r15, %rax
    movq %r14, %rcx
    addq %rcx, %rax
    movq %rax, %r9
    movq %r15, %rax
    movq %r14, %rcx
    subq %rcx, %rax
    movq %rax, %r8
    movq %r9, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r8
    movq %r12, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r12
    movq %r15, %rax
    movq %r14, %rcx
    imulq %rcx, %rax
    movq %rax, %r9
    movq %r14, %rax
    movq %r15, %rcx
    subq %rcx, %rax
    movq %rax, %r8
    movq %r9, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r8
    movq %r12, %rax
    movq %r8, %rcx
    addq %rcx, %rax
    movq %rax, %r12
    movq $1, %r8
    movq %r10, %rax
    movq %r8, %rcx
    subq %rcx, %rax
    movq %rax, %r13
    movq %r13, %r10
    jmp .Lmain_while_header_1
.Lmain_while_exit_3:
    movq %r12, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r10
    movq %r10, %rax
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

