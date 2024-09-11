.section .text
.globl array_max
.type array_max, @function

array_max:
    # Input: rdi = n (number of elements)
    #        rsi = *items (address of the first element of the array)

    # Base case: if n == 0, return immediately
    testq %rdi, %rdi
    je .Lreturn_zero

    # Initialize our max value to the first element of the array
    movq (%rsi), %rax   # rax will hold our max value

.Lloop_start:
    # If n is zero, we are done
    testq %rdi, %rdi
    jz .Ldone

    # Compare the current array element with our max value
    cmpq %rax, (%rsi)   # Corrected this line: compare max_val with array element
    
    # If array element > max_val, update max_val
    cmovg (%rsi), %rax

    # Move to the next array element
    addq $8, %rsi

    # Decrement our count n (rdi)
    decq %rdi

    # Repeat
    jmp .Lloop_start

.Ldone:
    # Return with max value in rax
    ret 

.Lreturn_zero:
    # If array length is zero, return 0 (though this is a bit redundant as we assume n >= 1, but kept for completeness)
    xorq %rax, %rax
    ret

