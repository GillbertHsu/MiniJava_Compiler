.section .data
.Lstr0: .asciz " "
.Lstr1: .asciz ""
.Lstr2: .asciz "The resuling matrix is"
s: .quad 0
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

matmul:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $128, %rsp
    movq %rdi, -104(%rbp)
    movq %rsi, -112(%rbp)
    movq -104(%rbp), %rax
    movq -8(%rax), %rax
    movq %rax, -136(%rbp)
    movq $0, %r10
    movq -104(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq -8(%rax), %rax
    movq %rax, -88(%rbp)
    movq -112(%rbp), %rax
    movq -8(%rax), %rax
    movq %rax, %r10
    movq $0, %r10
    movq -112(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq -8(%rax), %rax
    movq %rax, -96(%rbp)
    movq $0, %rax
    movq %rax, -72(%rbp)
    movq -136(%rbp), %rdi
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
    movq %rax, -80(%rbp)
    movq $0, %r13
    movq %r13, %r12
    jmp .Lmatmul_alloc2d_header_1
.Lmatmul_alloc2d_header_1:
    movq %r12, %rax
    movq -136(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_alloc2d_body_2
    jmp .Lmatmul_alloc2d_exit_3
.Lmatmul_alloc2d_body_2:
    movq -96(%rbp), %rdi
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
    movq -80(%rbp), %rax
    movq %r12, %rcx
    movq %rbx, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %rbx
    movq %rbx, %r12
    jmp .Lmatmul_alloc2d_header_1
.Lmatmul_alloc2d_exit_3:
    movq -80(%rbp), %rax
    movq %rax, -48(%rbp)
    movq -72(%rbp), %rax
    movq %rax, %r14
    jmp .Lmatmul_while_header_4
.Lmatmul_while_header_4:
    movq %r14, %rax
    movq -136(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_while_body_5
    jmp .Lmatmul_while_exit_6
.Lmatmul_while_body_5:
    movq $0, %r13
    movq %r13, %rax
    movq %rax, -160(%rbp)
    movq -48(%rbp), %rax
    movq %rax, -120(%rbp)
    jmp .Lmatmul_while_header_7
.Lmatmul_while_exit_6:
    movq s(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r13
    movq -160(%rbp), %rax
    movq %rax, %r10
    movq %r13, %r12
    jmp .Lmatmul_while_header_13
.Lmatmul_while_header_7:
    movq -160(%rbp), %rax
    movq -96(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_while_body_8
    jmp .Lmatmul_while_exit_9
.Lmatmul_while_body_8:
    movq $0, %rbx
    movq $0, %r12
    movq %r12, %r9
    movq %rbx, %r8
    jmp .Lmatmul_while_header_10
.Lmatmul_while_exit_9:
    movq $1, %r10
    movq %r14, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r14
    movq -120(%rbp), %rax
    movq %rax, -48(%rbp)
    jmp .Lmatmul_while_header_4
.Lmatmul_while_header_10:
    movq %r8, %rax
    movq -88(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_while_body_11
    jmp .Lmatmul_while_exit_12
.Lmatmul_while_body_11:
    movq -104(%rbp), %rax
    movq %r14, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r8, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r11
    movq -112(%rbp), %rax
    movq %r8, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq -160(%rbp), %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r11, %rax
    movq %r10, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r9, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -64(%rbp)
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -56(%rbp)
    movq -64(%rbp), %rax
    movq %rax, %r9
    movq -56(%rbp), %rax
    movq %rax, %r8
    jmp .Lmatmul_while_header_10
.Lmatmul_while_exit_12:
    movq -120(%rbp), %rax
    movq %r14, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq -160(%rbp), %rcx
    movq %r9, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq -160(%rbp), %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r15
    movq %r15, %rax
    movq %rax, -160(%rbp)
    movq -120(%rbp), %rax
    movq %rax, -120(%rbp)
    jmp .Lmatmul_while_header_7
.Lmatmul_while_header_13:
    movq %r12, %rax
    movq -136(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_while_body_14
    jmp .Lmatmul_while_exit_15
.Lmatmul_while_body_14:
    movq $0, %rbx
    movq %rbx, %rax
    movq %rax, -152(%rbp)
    jmp .Lmatmul_while_header_16
.Lmatmul_while_exit_15:
    movq $1, %r10
    movq %r10, %rax
    addq $128, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.Lmatmul_while_header_16:
    movq -152(%rbp), %rax
    movq -96(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmatmul_while_body_17
    jmp .Lmatmul_while_exit_18
.Lmatmul_while_body_17:
    movq -48(%rbp), %rax
    movq %r12, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rax
    movq -152(%rbp), %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lstr0(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $1, %r10
    movq -152(%rbp), %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -144(%rbp)
    movq -144(%rbp), %rax
    movq %rax, -152(%rbp)
    jmp .Lmatmul_while_header_16
.Lmatmul_while_exit_18:
    leaq .Lstr1(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, -128(%rbp)
    movq -152(%rbp), %rax
    movq %rax, %r10
    movq -128(%rbp), %rax
    movq %rax, %r12
    jmp .Lmatmul_while_header_13

main:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $32, %rsp
    movq %rdi, %rax
    movq %rsi, %r11
    movq $8, %r10
    movq %r11, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r11
    leaq .Lstr2(%rip), %r10
    movq %r10, %rax
    movq %rax, s(%rip)
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
    movq $2, %rax
    movq %rax, -56(%rbp)
    movq $3, %rax
    movq %rax, -64(%rbp)
    movq -56(%rbp), %rdi
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
    movq %rax, -48(%rbp)
    movq $0, %r13
    movq %r13, %r12
    jmp .Lmain_alloc2d_header_1
.Lmain_alloc2d_header_1:
    movq %r12, %rax
    movq -56(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_alloc2d_body_2
    jmp .Lmain_alloc2d_exit_3
.Lmain_alloc2d_body_2:
    movq -64(%rbp), %rdi
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
    movq -48(%rbp), %rax
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
    movq $1, %r11
    movq $0, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r11
    movq $0, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $2, %r11
    movq $0, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $2, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $4, %r11
    movq $1, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $3, %r11
    movq $1, %r10
    movq -48(%rbp), %rax
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
    movq -48(%rbp), %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $2, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq -64(%rbp), %rdi
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
    movq %rax, %r13
    movq $0, %r14
    movq %r14, %r12
    jmp .Lmain_alloc2d_header_4
.Lmain_alloc2d_header_4:
    movq %r12, %rax
    movq -64(%rbp), %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .Lmain_alloc2d_body_5
    jmp .Lmain_alloc2d_exit_6
.Lmain_alloc2d_body_5:
    movq -56(%rbp), %rdi
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
    movq %r13, %rax
    movq %r12, %rcx
    movq %rbx, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $1, %r10
    movq %r12, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r15
    movq %r15, %r12
    jmp .Lmain_alloc2d_header_4
.Lmain_alloc2d_exit_6:
    movq $2, %r11
    movq $0, %r10
    movq %r13, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $0, %r11
    movq $0, %r10
    movq %r13, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $3, %r11
    movq $1, %r10
    movq %r13, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $0, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq $2, %r11
    movq $1, %r10
    movq %r13, %rax
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
    movq %r13, %rax
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
    movq %r13, %rax
    movq %r10, %rcx
    movq (%rax,%rcx,8), %rax
    movq %rax, %r8
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    movq %r11, %rdx
    movq %rdx, (%rax,%rcx,8)
    movq -48(%rbp), %rdi
    movq %r13, %rsi
    call matmul
    movq %rax, %r10
    movq $0, %r10
    movq %r10, %rax
    addq $32, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

