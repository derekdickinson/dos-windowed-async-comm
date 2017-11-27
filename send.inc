lower_rts MACRO
         mov     dx,MDMCTRL    ; Lower Rts
         mov     al,9
         out     dx,al
         ENDM

kill_tx_ints MACRO
         mov     dx,INTENA     ; Disable tc interrupts
         in      al,dx
         and     al,0fdh
         out     dx,al
         ENDM

strt_out MACRO
         LOCAL notzero,notone

         mov     dx,TXHOLD
         mov     al,DLE
         out     dx,al

         mov     _txstate,SENDSTX
         ENDM

stx_out  MACRO
         LOCAL notzero,notone

         mov     dx,TXHOLD
         mov     al,STX
         out     dx,al

         mov     txindex,0
         mov     sendcrc,0

         mov     _txstate,SENDSEQ
         ENDM

seq_out  MACRO
         mov     al,_sendseq
         mov     dl,_sendack
         shlm    al,4
         or      al,dl

         mov     dx,TXHOLD
         out     dx,al
         mov     _txstate,SENDLEN0
         ENDM

len0_out MACRO
         mov     dx,TXHOLD
         mov     al,byte ptr _sendlen[0]
         out     dx,al
         mov     _txstate,SENDLEN1
         ENDM

len1_out MACRO
         LOCAL   notacksend

         mov     dx,TXHOLD
         mov     al,byte ptr _sendlen[1]
         out     dx,al

         cmp     _sendlen,0
         jne     notacksend      

         mov     _txstate,PACKDONE
         lower_rts
         kill_tx_ints
         jmp     theloop

notacksend:                      
         mov     _txstate,SENDATA
         ENDM

data_out MACRO
         LOCAL   more_to_send
         mov     bx,_outbuf

         mov     si,txindex
         inc     txindex
         mov     al,byte ptr [bx][si]

         and     ax,00ffh
         add     sendcrc, ax

         mov     dx,TXHOLD
         out     dx,al

         inc     si
         cmp     si,_sendlen
         jl      more_to_send

         mov     txindex,0
         mov     _txstate,SENDETX

more_to_send:
         ENDM

etx_out  MACRO
         mov     dx,TXHOLD
         mov     al,ETX
         out     dx,al
         mov     _txstate,SENDCRC0
         mov     txindex,0
         ENDM

crc0_out MACRO
         mov     dx,TXHOLD
         mov     al,byte ptr sendcrc[0]
         out     dx,al
         mov     _txstate,SENDCRC1
         ENDM

crc1_out MACRO
         mov     bx,_outbuf

         mov     dx,TXHOLD
         mov     al,byte ptr sendcrc[1]
         out     dx,al

         mov     _txstate,PACKDONE
         mov     txindex,0

         lower_rts
         kill_tx_ints
         ENDM
