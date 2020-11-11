.code
PAGE_SIZE equ 1000h
JIT_StackProbe PROC

        ; On entry:
        ;   r11 - points to the lowest address on the stack frame being allocated (i.e. [InitialSp - FrameSize])
        ;   rsp - points to some byte on the last probed page
        ; On exit:
        ;   rax - is not preserved
        ;   r11 - is preserved
        ;
        ; NOTE: this helper will probe at least one page below the one pointed by rsp.

        mov     rax, rsp               ; rax points to some byte on the last probed page
        and     rax, -PAGE_SIZE         ; rax points to the **lowest address** on the last probed page
                                       ; This is done to make the following loop end condition simpler.

ProbeLoop:
        sub     rax, PAGE_SIZE         ; rax points to the lowest address of the **next page** to probe
        test    dword ptr [rax], eax   ; rax points to the lowest address on the **last probed** page
        cmp     rax, r11
        jg      ProbeLoop              ; If (rax > r11), then we need to probe at least one more page.

        ret

JIT_StackProbe ENDP

end