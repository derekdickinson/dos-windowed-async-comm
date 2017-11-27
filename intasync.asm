        TITLE   intasync

_TEXT           SEGMENT  BYTE PUBLIC 'CODE'
_TEXT           ENDS
_DATA           SEGMENT  WORD PUBLIC 'DATA'
_DATA           ENDS
CONST           SEGMENT  WORD PUBLIC 'CONST'
CONST           ENDS
_BSS            SEGMENT  WORD PUBLIC 'BSS'
_BSS            ENDS

DGROUP  GROUP   CONST,  _BSS,   _DATA
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

; Defines and structure definitions.
        include equates.inc

; External declarations.
        include externs.inc

; Data Segment stuff.
        include data.inc

; SIO and interrupt macros.
        include stand.inc

; Status interrupt macros.
        include statints.inc

; Send macros.
        include send.inc

; Receive macros.
        include rcv.inc

_TEXT      SEGMENT

dseg      dw      0

mdmstseg  dw      0
mdmsp     dw      0

oldstseg  dw      0
oldsp     dw      0

         PUBLIC   _int_handler
_int_handler     PROC near

         startint

theloop:
         mov     dx,INTIDREG
         in      al,dx

         test    al,01
         jz      more_ints
         jmp     int_done

more_ints:

         and     ax,06h
         mov     si,ax
         jmp     intjmp[si]   ; Jump to appropriate routine

badint:
         errint
         jmp     theloop

txempt:
         mov     si,_txstate
         jmp     txjmp[si]

tx_idle:
         mov     txindex,0
         jmp     theloop
;
; Modem status interrupts.
;
mdmint:
         mdmint_do
                          ; jumps to appropriate routine below.
was_cts:
         cts_do
         jmp     theloop
was_dsr:
         dsr_do
         jmp     theloop
was_ri:
         ri_do
         jmp     theloop
was_dcd:
         dcd_do
         jmp     theloop
;
; Modem status interrupts.
;
rcpint:
         rcpint_do
                          ; jumps to appropriate routine below.
was_over:
         over_do
         jmp     theloop
was_party:
         party_do
         jmp     theloop
was_frerr:
         frerr_do
         jmp     theloop
was_break:
         break_do
         jmp     theloop

; Standard send mode labels.

tx_strt:
         strt_out
         jmp     theloop
tx_stx:
         stx_out
         jmp     theloop

tx_len0:
         len0_out
         jmp     theloop
tx_len1:
         len1_out
         jmp     theloop
tx_seq:
         seq_out
         jmp     theloop
tx_data:
         data_out
         jmp     theloop
tx_etx:
         etx_out
         jmp     theloop
tx_crc0:
         crc0_out
         jmp     theloop
tx_crc1:
         crc1_out
         jmp     theloop
;
; Done with tx section.  Now rx section.
;
rxchar:
         mov     dx,RXDATA      ; Get character.
         in      al,dx

         mov     si,_rxstate    ; Jump to appropriate process.
         jmp     rxjmp[si]

rx_idle:
         mov     rxindex,0
         jmp     theloop

;
; Receive macros
;
rx_strt:
         strt_in
         jmp     theloop

rx_stx:
         stx_in
         jmp     theloop

rx_seq:
         seq_in
         jmp     theloop

rx_len0:
         len_in0
         jmp     theloop

rx_len1:
         len_in1
         jmp     theloop

rx_data:
         data_in
         jmp     theloop

rx_etx:
         etx_in
         jmp     theloop

rx_crc0:
         crc0_in
         jmp     theloop

rx_crc1:
         crc1_in
         jmp     theloop

;
; Normal cleanup stuff
;

int_done:
         endint
         iret
_int_handler    ENDP


         PUBLIC  _setseg
_setseg  PROC    near
         mov     cs:dseg,ds

         mov     cs:mdmstseg,SEG _mdmstack
         mov     cs:mdmsp,OFFSET _mdmstack

         add     cs:mdmsp,800

         mov     _cseg_intl,SEG _int_handler
         ret
_setseg  ENDP

_TEXT   ENDS
END
