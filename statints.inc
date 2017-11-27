
mdmint_do MACRO
         mov     dx,MDMSTUS
         in      al,dx        ; Get Modem Control register

         and     ax,000fh
         shl     ax,1
         mov     si,ax
         jmp     mdmjmp[si]   ; Jump to appropriate routine.
         ENDM

cts_do   MACRO
         ENDM

dsr_do   MACRO
         ENDM

ri_do    MACRO
         ENDM

dcd_do   MACRO
         ENDM


;--------------------------------------------------------------------------

rcpint_do MACRO
         mov     dx,LINESTUS
         in      al,dx        ; Get Modem Control register

         and     ax,001eh

         mov     si,ax
         jmp     rcpjmp[si]   ; Jump to appropriate routine.
         ENDM

break_do MACRO
         ENDM

frerr_do MACRO
         ENDM

party_do MACRO
         ENDM

over_do  MACRO
         ENDM

errint   MACRO
badm0000:
badm0011:
badm0101:
badm0110:
badm0111:
badm1001:
badm1010:
badm1011:
badm1100:
badm1101:
badm1110:
badm1111:
badr0000:
badr0011:
badr0101:
badr0110:
badr0111:
badr1001:
badr1010:
badr1011:
badr1100:
badr1101:
badr1110:
badr1111:
         ENDM
