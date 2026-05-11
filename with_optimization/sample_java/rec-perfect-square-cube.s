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

findPerfectSquareAndCubes:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $16, %rsp
    movq %rdi, %r14
    movq %rsi, -48(%rbp)
    movq %rdx, %r13
    movq %r13, %rax
    movq %r14, %rcx
    cmpq %rcx, %rax
    setl %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LfindPerfectSquareAndCubes_if_then_1
    jmp .LfindPerfectSquareAndCubes_if_else_2
.LfindPerfectSquareAndCubes_if_then_1:
    movq $1, %r10
    movq -48(%rbp), %rdi
    movq %r10, %rsi
    call isSquare
    movq %rax, %r12
    movq $1, %r10
    movq -48(%rbp), %rdi
    movq %r10, %rsi
    call isCube
    movq %rax, %rbx
    movq %r12, %rax
    movq %rbx, %rcx
    andq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LfindPerfectSquareAndCubes_if_then_4
    jmp .LfindPerfectSquareAndCubes_if_else_5
.LfindPerfectSquareAndCubes_if_else_2:
    movq $1, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    subq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    addq $16, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.LfindPerfectSquareAndCubes_if_merge_3:
.LfindPerfectSquareAndCubes_if_then_4:
    movq $1, %r10
    movq %r13, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r15
    movq %r15, %r11
    jmp .LfindPerfectSquareAndCubes_if_merge_6
.LfindPerfectSquareAndCubes_if_else_5:
    movq %r13, %r11
    jmp .LfindPerfectSquareAndCubes_if_merge_6
.LfindPerfectSquareAndCubes_if_merge_6:
    movq $1, %r10
    movq -48(%rbp), %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r10
    movq %r14, %rdi
    movq %r10, %rsi
    movq %r11, %rdx
    call findPerfectSquareAndCubes
    movq %rax, %rbx
    movq %rbx, %rax
    addq $16, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

isSquare:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    movq %rdi, %r8
    movq %rsi, %r11
    movq %r11, %rax
    movq %r11, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r8, %rcx
    cmpq %rcx, %rax
    setg %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisSquare_if_then_1
    jmp .LisSquare_if_else_2
.LisSquare_if_then_1:
    movq $0, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret
.LisSquare_if_else_2:
    movq %r11, %rax
    movq %r11, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r8, %rcx
    cmpq %rcx, %rax
    sete %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisSquare_if_then_4
    jmp .LisSquare_if_else_5
.LisSquare_if_merge_3:
.LisSquare_if_then_4:
    movq $1, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret
.LisSquare_if_else_5:
    movq $1, %r10
    movq %r11, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r10
    movq %r8, %rdi
    movq %r10, %rsi
    call isSquare
    movq %rax, %rbx
    movq %rbx, %rax
    popq %rbx
    popq %rbp
    ret
.LisSquare_if_merge_6:
    jmp .LisSquare_if_merge_3

isCube:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    movq %rdi, %r11
    movq %rsi, %r8
    movq %r8, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r11, %rcx
    cmpq %rcx, %rax
    setg %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisCube_if_then_1
    jmp .LisCube_if_else_2
.LisCube_if_then_1:
    movq $0, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret
.LisCube_if_else_2:
    movq %r8, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r8, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    movq %r11, %rcx
    cmpq %rcx, %rax
    sete %al
    movzbq %al, %rax
    movq %rax, %r10
    movq %r10, %rax
    testq %rax, %rax
    jne .LisCube_if_then_4
    jmp .LisCube_if_else_5
.LisCube_if_merge_3:
.LisCube_if_then_4:
    movq $1, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret
.LisCube_if_else_5:
    movq $1, %r10
    movq %r8, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r10
    movq %r11, %rdi
    movq %r10, %rsi
    call isCube
    movq %rax, %rbx
    movq %rbx, %rax
    popq %rbx
    popq %rbp
    ret
.LisCube_if_merge_6:
    jmp .LisCube_if_merge_3

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
    movq $1, %r11
    movq $0, %r10
    movq %rbx, %rdi
    movq %r11, %rsi
    movq %r10, %rdx
    call findPerfectSquareAndCubes
    movq %rax, %rbx
    movq %rbx, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq $0, %r10
    movq %r10, %rax
    popq %rbx
    popq %rbp
    ret

