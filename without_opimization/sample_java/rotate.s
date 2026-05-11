.section .data
.Lstr0: .asciz "Before rotation"
.Lstr1: .asciz "After rotation"
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
    movq %rax, -88(%rbp)
    movq $1, %r10
    movq $0, %r11
    movq -88(%rbp), %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r10
    movq $1, %r11
    movq -88(%rbp), %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r10
    movq $2, %r11
    movq -88(%rbp), %rax
    movq %r11, %rcx
    movq %r10, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $3, %r13
    movq $3, %r14
    movq %r13, %rdi
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
    movq %rax, -72(%rbp)
    movq $0, %r15
    movq %r15, %r12
    jmp .Lmain_alloc2d_header_1
.Lmain_alloc2d_header_1:
    movq %r12, %rax
    movq %r13, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_alloc2d_body_2
    jmp .Lmain_alloc2d_exit_3
.Lmain_alloc2d_body_2:
    movq %r14, %rdi
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
    movq -72(%rbp), %rax
    movq %r12, %rcx
    movq %rbx, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %rbx
    movq %rbx, %r12
    jmp .Lmain_alloc2d_header_1
.Lmain_alloc2d_exit_3:
    movq $0, %r11
    movq $0, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq %r10, %rax
    negq %rax
    movq %rax, %r11
    movq $0, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $0, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $2, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r11
    movq $1, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $1, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $1, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $2, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $2, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $2, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r11
    movq $2, %r10
    movq -72(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $2, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
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
    movq %rax, %r15
    movq $0, %r14
    movq %r14, %r13
    movq %r15, %rax
    movq %rax, -104(%rbp)
    jmp .Lmain_while_header_4
.Lmain_while_header_4:
    movq $3, %r10
    movq %r13, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_5
    jmp .Lmain_while_exit_6
.Lmain_while_body_5:
    movq $0, %rbx
    movq $0, %r12
    movq %rbx, %r9
    movq %r12, %r8
    jmp .Lmain_while_header_7
.Lmain_while_exit_6:
    leaq .Lstr0(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r12
    movq %r12, %rbx
    jmp .Lmain_while_header_10
.Lmain_while_header_7:
    movq $3, %r10
    movq %r9, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_8
    jmp .Lmain_while_exit_9
.Lmain_while_body_8:
    movq -72(%rbp), %rax
    movq %r13, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r9, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r11
    movq -88(%rbp), %rax
    movq %r9, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r11, %rax
    movq %r10, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r8, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -64(%rbp)
    movq $1, %r10
    movq %r9, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -56(%rbp)
    movq -56(%rbp), %rax
    movq %rax, %r9
    movq -64(%rbp), %rax
    movq %rax, %r8
    jmp .Lmain_while_header_7
.Lmain_while_exit_9:
    movq -104(%rbp), %rax
    movq %r13, %rcx
    movq %r8, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq %r13, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -48(%rbp)
    movq -48(%rbp), %rax
    movq %rax, %r13
    movq -104(%rbp), %rax
    movq %rax, -104(%rbp)
    jmp .Lmain_while_header_4
.Lmain_while_header_10:
    movq $3, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_11
    jmp .Lmain_while_exit_12
.Lmain_while_body_11:
    movq -88(%rbp), %rax
    movq %rbx, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $1, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -80(%rbp)
    movq -80(%rbp), %rax
    movq %rax, %rbx
    jmp .Lmain_while_header_10
.Lmain_while_exit_12:
    leaq .Lstr1(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r12
    movq %r12, %rbx
    jmp .Lmain_while_header_13
.Lmain_while_header_13:
    movq $3, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_while_body_14
    jmp .Lmain_while_exit_15
.Lmain_while_body_14:
    movq -104(%rbp), %rax
    movq %rbx, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $1, %r10
    movq %rbx, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -96(%rbp)
    movq -96(%rbp), %rax
    movq %rax, %rbx
    jmp .Lmain_while_header_13
.Lmain_while_exit_15:
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

