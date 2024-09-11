global main
.extern compare, atol, printf

.section .data
error_msg: .asciz "Two arguments required.\n"
format_less: .asciz "less\n"
format_equal: .asciz "equal\n"
format_greater: .asciz "greater\n"

.section .text
main:
    cmp $3, %rdi
    jne error_exit
    mov %rsi, %rdx
    mov 8(%rdx), %rdi
    call atol
    mov %rax, %rdi

    mov 16(%rdx), %rsi
    call atol
    mov %rax, %rsi
    call compare

    cmp $-1, %rax
    je print_less
    cmp $0, %rax
    je print_equal

    mov $format_greater, %rdi
    call printf
    ret

print_less:
    mov $format_less, %rdi
    call printf
    ret

print_equal:
    mov $format_equal, %rdi
    call printf
    ret

error_exit:
    mov $error_msg, %rdi
    call printf
    mov $1, %rax
    ret
