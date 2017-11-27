
shrm     MACRO which,num
         REPT num
         shr &which,1
         ENDM
         ENDM

shlm     MACRO which,num
         REPT num
         shl &which,1
         ENDM
         ENDM

startint MACRO
         mov     cs:oldstseg,ss
         mov     cs:oldsp,sp
         mov     ss,cs:mdmstseg
         mov     sp,cs:mdmsp
         sti
         push    ax
         push    bx
         push    cx
         push    dx
         push    di
         push    si
         push    ds
         push    es
         mov     ds,cs:dseg
         ENDM

endint   MACRO
         pop     es
         pop     ds
         pop     si
         pop     di
         pop     dx
         pop     cx
         pop     bx
         mov     al,20h
         cli
         out     20h,al
         pop     ax
         mov     ss,cs:oldstseg
         mov     sp,cs:oldsp
         sti
         ENDM
