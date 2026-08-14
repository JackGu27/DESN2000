        AREA    decision_code, CODE, READONLY
        EXPORT  decision_asm
        CODE32

    
; =========================================================
; decision_asm
;
; R0 = light sensor reading
; R1 = minutes after midnight
;
; Return:
; bits 1:0 = blind position
; bit 2    = smart plug
;
; led.h convention:
; UP   = 0
; MID  = 1
; DOWN = 2
; =========================================================

decision_asm
    
; PART 1 BLIND AUTOMATION
        ; Start by assuming the blind is UP.
        MOV     R2, #2

        ; Compare light value with dark threshold.
        ; light < 600 -> keep blind UP
        LDR     R3, =600
        CMP     R0, R3
        BLT     blind_done
        
        ; Light is now >= 600.
        ; Check second threshold, light < 2000 -> keep blind MID.
        LDR     R3, =2000
        CMP     R0, R3
        BLT     blind_mid

        ; light >= 2000
        ; Very bright environment.
        ; Close blind to reduce glare / heat gain, keep blind DOWN.
        MOV     R2, #0
        B       blind_done

blind_mid      
        ; Set blind MID.
        MOV     R2, #1

; PART 2 SMART PLUG AUTOMATION
blind_done
        ; Start with plug OFF.
        MOV     R3, #0

        ; Preheat schedule starts at 15:30.
        ; 15:30 = 15 * 60 + 30 = 930 minutes
        LDR     R12, =930
        CMP     R1, R12

        ; Current time is before 15:30, Keep plug OFF.
        BLT     pack_result
        
        ; End of preheat period = 16:30.
        ; 16:30 = 16 * 60 + 30 = 990 minutes
        LDR     R12, =990
        CMP     R1, R12

        ; If time >= 16:30, plug stays OFF.
        BGE     pack_result

        ; 930 <= time < 990, Turn plug ON.
        MOV     R3, #1

; PART 3 PACK RETURN VALUE
pack_result
        ; R2 contains blind: DOWN = 0 MID = 1 UP = 2
        ; R3 contains plug: OFF = 0 ON  = 1
        ; Shift plug state left by 2 bits, Then combine it with blind state.

        ; Result: bit 2 = plug, bits 1:0 = blind
        ORR     R0, R2, R3, LSL #2

        ; Return to C.
        BX      LR
        END
