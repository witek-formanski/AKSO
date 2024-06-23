global _start

SYS_READ equ 0
SYS_WRITE equ 1
SYS_OPEN equ 2
SYS_CLOSE equ 3
SYS_LSEEK equ 8
SYS_EXIT equ 60

SEEK_CUR equ 1                     ; The offset in SYS_LSEEK is set to current location plus offset.
STDOUT equ 1
O_RDONLY equ 0
ASCII_ZERO equ 48                  ; '0' in ASCII table.
ASCII_NEWLINE equ 10               ; '\n' in ASCII table.

SUCCESS_EXIT_CODE equ 0
ERROR_EXIT_CODE equ 1

section .bss
    buf resq 8192                  ; Buffer for data (to contain maximal possible data fragment).
    tab resq 256                   ; Space for lookup table.

section .text

; This program reads data from "file with holes" and calculates CRC.
; Parameters:
; - path to input file
; - polynomial to calculate CRC (in binary)
; Output:
; - CRC result

_start:
    mov rcx, [rsp]                 ; Load parameters count to rcx.
    cmp rcx, 3                     ; First parameter (index 0) is current file name, second - input file name and third - CRC polynomial.
    jne _error_exit                ; If different number of parameters is provided, exit with error code.

                                   ; Convert polynomial string to integer.

_poly_str_to_int_start:
    mov rsi, qword [rsp + 24]      ; Load pointer to polynomial string to rsi.
    xor eax, eax                   ; Clear rax - it will be used for conversion.
    xor r12d, r12d                 ; Clear r12 - it will store CRC polynomial as integer.
    xor r10d, r10d                 ; Clear r10 - it will store length-1.
    mov r9d, 64                    ; 64 minus polynomial length will be stored in r9.

_poly_str_to_int_loop:
    lodsb                          ; Load byte at address in rsi into al and increment rsi.
    test al, al                    ; Test if al is zero (end of string).
    jz _poly_str_to_int_end        ; If zero, end conversion.

    sub al, ASCII_ZERO             ; Convert from ASCII to binary.

    test al, al                    ; If ASCII value less than '0', error.
    js _error_exit

    cmp al, 1                      ; If ASCII value greater than '1', error.
    jg _error_exit

    shl r12, 1                     ; Shift left r12 to make room for next bit.
    dec r9b                        ; Decrease counter.
    inc r10b                       ; Increase r10 to calculate length.
    add r12, rax                   ; Add LSB to r12.
    jmp _poly_str_to_int_loop      ; Repeat.

_poly_str_to_int_end:
    mov rcx, r9                    ; Copy (64-length) to rcx to use shl.
    shl r12, cl                    ; The CRC polynomial will always be of degree 64.

                                   ; Create lookup table.

_create_lookup_table_start:
    mov rcx, 255                   ; Set loop counter to 255. It will also be index in table.

_create_lookup_table_loop:
    mov rax, rcx                   ; In rax will be stored lookup value.
    shl rax, 56                    ; Shift left by 56.
    mov r8, 8                      ; Set nested loop counter to 8.

_generate_lookup_value:
    shl rax, 1                     ; Shift left once.
    jnc _decrease_nested_counter   ; If no carry flag, skip xor.
    xor rax, r12                   ; XOR with CRC polynomial.

_decrease_nested_counter:
    dec r8                         ; Decrease counter.
    test r8, r8
    jnz _generate_lookup_value

_add_element:
    mov [tab + 8 * rcx], rax       ; Add elemet to lookup table.
    loop _create_lookup_table_loop ; Repeat.

                                   ; Open file.

_open_file:
    mov eax, SYS_OPEN
    mov rdi, [rsp + 16]            ; File name.
    xor edx, edx                   ; O_RDONLY mode.
    xor esi, esi                   ; No flags
    syscall                        ; The file descriptor is returned to rax.

    test rax, rax                  ; If error occurs, rax is negative.
    js _close_and_error_exit

                                   ; Read data.

_prepare_for_reading:
    mov rdi, rax                   ; Move file descriptor to rdi.
    xor r12, r12                   ; Set position to 0.

_read_fragment_size:
    mov rax, SYS_READ
    mov edx, 2                     ; Read two bytes (size of fragment).
    mov rsi, buf                   ; Use the same buffer as for data.
    syscall

    cmp rax, 2                     ; If rax < 2, the error has occured.
    js _close_and_error_exit

_read_fragment:
    mov rax, SYS_READ
    movzx rdx, word [buf]          ; Use the size that was read before.
    mov rsi, buf                   ; Reuse buffer.
    syscall

    test rax, rax                  ; If error occurs, rax is negative.
    js _close_and_error_exit

_test_data_size:
    test rdx, rdx
    jz _read_offset                ; If there was no data, skip crc calculation.

_prepare_crc_counter:
    xor ecx, ecx                   ; Set counter to 0.

                                   ; Calculate CRC.

_calculate_crc_loop:
    movzx rax, byte [buf + rcx]
    shl rax, 56                    ; Shift data 56 bits left.
    xor rax, r8                    ; XOR registers. In r8 will be stored CRC result.
    shl r8, 8                      ; Shift result 8 bits left.
    shr rax, 56                    ; Shift data 56 bits right.
    xor r8, [tab + 8 * rax]        ; XOR with value from lookup table.

    inc rcx                        ; Increase counter.
    cmp rcx, rdx                   ; Loop until counter reaches data size.
    jl _calculate_crc_loop

_read_offset:
    mov rax, SYS_READ
    mov rdx, 4                     ; Read 4 bytes of offset.
    syscall

    cmp rax, 4                     ; If less than 4 bytes were read or rax is negative, it's an error.
    jl _close_and_error_exit

_get_position:
    mov rax, SYS_LSEEK
    movsxd rsi, dword [buf]        ; Set offset.
    mov rdx, SEEK_CUR              ; Seek from current position.
    syscall

    test rax, rax                  ; If error occurs, rax is negative.
    js _close_and_error_exit

    cmp rax, r12                   ; Compare with previous position.
    mov r12, rax                   ; Save current position to r12.
    jne _read_fragment_size        ; If the positions are not equal, continue.

                                   ; Convert result to string and print it.

_prepare_for_converstion:
    mov rcx, r9                    ; Set rcx to 64-length.
    shr r8, cl                     ; Shift result back to original degree of polynomial.

    mov rcx, r10                   ; Set rcx to length - it will be loop counter.
    inc r10b                       ; Set r10 to length+1.

    lea rsi, [buf + 64]            ; The string will be stored in rsi. It is at most 65 chars long (including newline).
    mov byte [rsi], ASCII_NEWLINE  ; Add '\n' at the end of string.

_result_integer_to_binary_string_loop:
    dec rsi                        ; Move to previous position in string.
    shr r8, 1                      ; Shift right result and affect CF.
    setc al                        ; Set al to carry flag value.
    add al, ASCII_ZERO             ; Convert al from int to char.
    mov byte [rsi], al             ; Add char to string.
    loop _result_integer_to_binary_string_loop

_print:
    mov rax, SYS_WRITE
    mov rdi, STDOUT                ; File descriptor 1 is stdout.
    mov rdx, r10                   ; Length of string.
    syscall

_success:
    mov rax, SYS_CLOSE
    syscall

    test rax, rax
    js _error_exit

    mov rax, SYS_EXIT
    mov rdi, SUCCESS_EXIT_CODE     ; Set exit code to 0.
    syscall

_close_and_error_exit:
    mov rax, SYS_CLOSE
    syscall

_error_exit:
    mov rax, SYS_EXIT
    mov rdi, ERROR_EXIT_CODE       ; Set exit code to 1.
    syscall
