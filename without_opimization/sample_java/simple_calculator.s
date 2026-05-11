.section .data
.Lstr0: .asciz "Sum: "
.Lstr1: .asciz "Difference: "
.Lstr2: .asciz "Product: "
.Lstr3: .asciz "x = "
.Lstr4: .asciz "y = "
s: .quad 0
d: .quad 0
p: .quad 0
x: .quad 0
y: .quad 0
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

add:
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp
    movq %rdi, %r11
    movq %rsi, %r10
    movq %r11, %rax
    movq %r10, %rcx
    addq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    addq $8, %rsp
    popq %rbp
    ret

subtract:
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp
    movq %rdi, %r11
    movq %rsi, %r10
    movq %r11, %rax
    movq %r10, %rcx
    subq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    addq $8, %rsp
    popq %rbp
    ret

multiply:
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp
    movq %rdi, %r11
    movq %rsi, %r10
    movq %r11, %rax
    movq %r10, %rcx
    imulq %rcx, %rax
    movq %rax, %r10
    movq %r10, %rax
    addq $8, %rsp
    popq %rbp
    ret

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
    leaq .Lstr0(%rip), %r10
    movq %r10, %rax
    movq %rax, s(%rip)
    leaq .Lstr1(%rip), %r10
    movq %r10, %rax
    movq %rax, d(%rip)
    leaq .Lstr2(%rip), %r10
    movq %r10, %rax
    movq %rax, p(%rip)
    movq $19, %r10
    movq %r10, %rax
    movq %rax, x(%rip)
    movq $10, %r10
    movq %r10, %rax
    movq %rax, y(%rip)
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
    leaq .Lstr3(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq x(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lstr4(%rip), %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq y(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq s(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq x(%rip), %rax
    movq %rax, %r11
    movq y(%rip), %rax
    movq %rax, %r10
    movq %r11, %rdi
    movq %r10, %rsi
    call add
    movq %rax, %rbx
    movq %rbx, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq d(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq x(%rip), %rax
    movq %rax, %r11
    movq y(%rip), %rax
    movq %rax, %r10
    movq %r11, %rdi
    movq %r10, %rsi
    call subtract
    movq %rax, %rbx
    movq %rbx, %rsi
    leaq .Lfmt_int(%rip), %rdi
    xorl %eax, %eax
    call printf
    leaq .Lfmt_nl(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq p(%rip), %rax
    movq %rax, %r10
    movq %r10, %rsi
    leaq .Lfmt_str(%rip), %rdi
    xorl %eax, %eax
    call printf
    movq x(%rip), %rax
    movq %rax, %r11
    movq y(%rip), %rax
    movq %rax, %r10
    movq %r11, %rdi
    movq %r10, %rsi
    call multiply
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

