section .text
global mdiv

mdiv:
; Parameters:
;   rdi = x[0]  - table storing dividend
;   rsi = n     - size of table
;   rdx = y     - divisor
; Returns:
;   rax - remainder
;   rdi - quotient (in the same table as dividend earlier)
.start:
    mov r8, rdx ; Assign divisor value to r8, because rdx will be in use.
    xor r9b, r9b ; Set r9b to zero to indicate that there was no negation so far.

.negate_divisor:
    mov r10, r8 ; Save original divisor in r10 to remember its sign.
    test r8, r8 ; Check divisor sign.
    jns .check_dividend_sign ; If the divisor is positive, skip negation.
    neg r8 ; If the divisor is negative, negate it.

.check_dividend_sign:
    mov r11, qword [rdi + 8 * rsi - 8] ; Save dividend most sigificant bits in r11 to remember its sign.
    test r11, r11 ; Check dividend sign.
    jns .divide ; If dividend is not negative, skip negation.

.negate_array:
    mov rcx, rsi ; Copy rsi to rcx for the loop counter (loop decrements rcx).
    xor edx, edx ; Clear rdx to 0.
    stc ; Set carry flag to always add 1 to the least significant bit.
.negate_loop:
    inc rdx ; Increment counter.
    not qword [rdi + 8 * rdx - 8] ; Inverse bits.
    adc qword [rdi + 8 * rdx - 8], 0 ; Handle carry (add 1 if needed).
    loop .negate_loop

.after_negation:
    test r9b, r9b ; Check if it is first or second negation of array.
    jnz .end ; If second, end execution.

.divide:
    mov rcx, rsi ; Set loop counter to n.
    xor edx, edx ; Ensure rdx is zero (remainder will be stored in it).
.divide_loop:
    mov rax, qword [rdi + 8 * rcx - 8] ; Load part of dividend.
    div r8 ; Divide rdx:rax by r8.
    mov qword [rdi + 8 * rcx - 8], rax ; Save result.
    loop .divide_loop

.correct_remainder_sign:
    mov rax, rdx ; Copy remainder to rax.
    test r11, r11 ; Check original sign of dividend - the sign of remainder should be the same.
    jns .correct_result_sign ; If dividend was positive, don't negate.
    neg rax

.correct_result_sign:
    xor r10, r11 ; If divider and dividend were of different signs, don't negate result.
    sets r9b ; Indicate, that it's second negation.
    js .negate_array

.check_overflow: ; Overflow (other than division by zero) is only possible when minimal integer in range is divided by -1.
    bt qword [rdi + 8 * rsi - 8], 63 ; Check if result is negative.
    jc .overflow  ; If the result is negative, but dividend and divisor were of the same signs, it's overflow.

.end:
    ret ; Return remainder in rax.

.overflow:
    div cl ; Divide by 0 (rcx is counter from division and it's set to 0).
