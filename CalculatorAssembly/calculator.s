.global main
.text

atoi:
  xor %rax, %rax
.next_digit:
  movzx (%rdi), %rdx
  test %rdx, %rdx
  jz .done
  sub $'0', %rdx
  imulq $10, %rax
  addq %rdx, %rax
  inc %rdi
  jmp .next_digit
.done:
  ret

main:
  enter $0, $0

  movq 8(%rsi), %r10
  movq 16(%rsi), %rdi
  call atoi
  movq %rax, %r11

  movq 24(%rsi), %rdi
  call atoi
  movq %rax, %r12

  movb (%r10), %al

  cmp $'+', %al
  je addition

  cmp $'-', %al
  je subtraction

  cmp $'*', %al
  je multiplication

  cmp $'/', %al
  je division

  jmp error

addition:
  addq %r12, %r11
  jmp print_result

subtraction:
  subq %r12, %r11
  jmp print_result

multiplication:
  imulq %r12, %r11
  jmp print_result

division:
  test %r12, %r12
  jz division_by_zero
  xor %rdx, %rdx
  divq %r12
  movq %rax, %r11
  jmp print_result

division_by_zero:
  movq $format_error_division, %rdi
  call printf
  jmp end

error:
  movq $format_error, %rdi
  call printf
  jmp end

print_result:
  movq $format, %rdi
  movq %r11, %rsi
  call printf
  jmp end

end:
  movq $0, %rax
  leave
  ret

.data

format: 
  .asciz "%ld\n"

format_error:
  .asciz "Error: Unknown operation\n"

format_error_division:
  .asciz "Error: Division by zero\n"

