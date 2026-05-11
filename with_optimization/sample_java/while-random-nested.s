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
    subq $64, %rsp
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
    movq %rax, -96(%rbp)
    movq $1, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, -104(%rbp)
    movq $2, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, -80(%rbp)
    movq $3, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    xorl %eax, %eax
    movq %r10, %rdi
    call atoi
    movq %rax, -64(%rbp)
    movq $0, %r14
    movq $0, %r13
    movq $0, %r15
    movq %r14, %r12
    movq %r13, %rbx
    movq %r15, %r9
    jmp .Lmain_while_header_1
.Lmain_while_header_1:
    movq %r12, %rax
    movq -96(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_2
    jmp .Lmain_while_exit_3
.Lmain_while_body_2:
    movq %rbx, %r8
    movq %r9, %r11
    jmp .Lmain_while_header_4
.Lmain_while_exit_3:
    movq %r9, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r10
    movq %r10, %rax
    addq $64, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.Lmain_while_header_4:
    movq %r8, %rax
    movq -104(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_5
    jmp .Lmain_while_exit_6
.Lmain_while_body_5:
    movq %r11, %rax
    movq -64(%rbp), %rcx
    addq %rcx, %rax
    movq %rax, -88(%rbp)
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -72(%rbp)
    movq -72(%rbp), %rax
    movq %rax, %r8
    movq -88(%rbp), %rax
    movq %rax, %r11
    jmp .Lmain_while_header_4
.Lmain_while_exit_6:
    movq $0, %rax
    movq %rax, -48(%rbp)
    movq %r11, %rax
    movq -80(%rbp), %rcx
    addq %rcx, %rax
    movq %rax, -56(%rbp)
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r12
    movq -48(%rbp), %rax
    movq %rax, %rbx
    movq -56(%rbp), %rax
    movq %rax, %r9
    jmp .Lmain_while_header_1

