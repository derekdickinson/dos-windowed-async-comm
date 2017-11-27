;       Static Name Aliases
;
        TITLE   scrnlib

_TEXT   SEGMENT  BYTE PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS
CONST   SEGMENT  WORD PUBLIC 'CONST'
CONST   ENDS
_BSS    SEGMENT  WORD PUBLIC 'BSS'
_BSS    ENDS
DGROUP  GROUP   CONST,  _BSS,   _DATA
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

_DATA   SEGMENT
        public  _video_base
_video_base   DW    0B800h
        public  _video_attr
_video_attr   DW    0Eh
        public  _vidmode
_vidmode      DW    03h
        public  _inv_mask
_inv_mask     DW    8F20h
        public  _clr_mask
_clr_mask     DW    8F00h
_DATA   ENDS

_TEXT      SEGMENT

        PUBLIC  _fasrite
_fasrite        PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;col
                shl     cx,1
                add     ax,cx
                sub     ax,162
                mov     di,ax           ;offs

                mov     es,_video_base  ;put base address in es for STOSW
                mov     si,[bp+8]       ;strg address for lodsb instruct
                mov     ah,BYTE PTR _video_attr

                jmp     SHORT $fas2
          $fas1:
                stosw                 ;store word (ah=video_attr) to screen (loc es:si)
          $fas2:
                lodsb                 ;load byte from ds:di string and auto inc di
                or      al,al         ;set up for comparing al to zero
                jne     $cmpff
                jmp     $thend

          $cmpff:
                cmp     al,07fh        ;Check for graphics character or FF
                jbe     $fas1          ;jump if neither
                cmp     al,0ffh        ;check for FF (Esc sequence)
                je      $esc_seq       ;jmp if not
                cmp     ah,0bh
                je      $fas1
                cmp     byte ptr _vidmode,3
                jne     $fas1
                mov     ch,ah
                mov     ah,0ah
                stosw
                mov     ah,ch
                jmp     $fas2

       $esc_seq:
                lodsb

                cmp     al,1            ;Check for new Attribute sequence
                je      $is1

                cmp     al,2            ;Check for new ro,col location
                je      $is2

                cmp     al,3            ;Check for continuation char
                je      $is3

                xor     ch,ch           ;must be repeated char sequence
                mov     cl,al
                mov     dh,ah           ;save old att
                cmp     byte ptr _vidmode,3
                jne     nocolor
                mov     ah,0ah          ;make green background att
        nocolor:
                lodsb
                rep     stosw
                mov     ah,dh           ;restore attribute
                jmp     $fas2           ;get next char

           $is1:                        ;New Attribute code
                lodsb                   ;attribute loaded into al
                mov     cl,BYTE PTR _vidmode
                cmp     cl,3
                jne     $noclr
                mov     cl,8
                ror     ax,cl
                jmp     $fas2
         $noclr:
                cmp     al,0Bh
                je      $doit
                mov     ah,07h
                jmp     $fas2
          $doit:
                mov     ah,0Fh
                jmp     $fas2

           $is2:
                lodsb                   ;gorocol sequence
                mov     bh,ah           ;store ah (video_attr) in bh
                mov     cl,160
                mul     cl              ;multiply ro by 160
                mov     cx,ax
                lodsb
                xor     ah,ah
                shl     ax,1
                add     ax,cx           ;add 2 times the col
                sub     ax,162          ;subtract 162 from result
                mov     di,ax           ;set di for stosw intruction
                mov     ah,bh           ;restore in bh (video_attr) to ah
                jmp     $fas2

           $is3:                      ;Extended Escape, find out which
                lodsb
                cmp     al,01
                je      $skip         ;Skip locations sequence

                cmp     al,02
                je      $putcur       ;move flashing cursor to here sequence

                cmp     al,03         ;Turn reverse mode on
                je      $revmode

                cmp     al,04         ;Turn reverse mode off
                je      $unrev

                cmp     al,05         ;repeat last line
                jne     $+5
                jmp     $replin

                cmp     al,06         ;Higlight in Mono Mode
                jne     $+5
                jmp     $hilite

                cmp     al,07         ;underline in Mono Mode
                jne     $+5
                jmp     $undline

                cmp     al,10
                je      $lf           ;Line Feed sequence

                mov     [bp+4],al     ;store value for later check
                                      ;al = 13 or 14

                add     di,160        ;implement CR-LF sequence, add 160 for LF
                mov     ch,ah         ;save attribute
                mov     ax,di         ;Set up for divide
                mov     cl,160        ;Set up for divide
                div     cl            ;divide by 160
                mul     cl            ;multiply by 160 (remainder is lost)
                mov     di,ax         ;put value back in di for stosw
                mov     ah,ch         ;restore attribute
                cmp     byte ptr [bp+4],14
                je      $+5
                jmp     $fas2
                add     di,4          ;start in column three instead of one.
                jmp     $fas2

          $skip:
                lodsb                 ;skip this number of locations
                mov     ch,ah         ;save ah video attribute in ch
                xor     ah,ah         ;zero ah for Add
                shl     ax,1          ;multiply by 2
                add     di,ax         ;increment stosw pointer
                mov     ah,ch         ;restore attribute
                jmp     $fas2

            $lf:
                add     di,158        ;perform a linfeed-2
                jmp     $fas2

        $putcur:                      ;putcur will go here
                mov     ch,ah
                mov     ax,di
                mov     cl,160
                div     cl
                shr     ah,1
                xor     bh,bh
                mov     cl,8
                ror     ax,cl
                mov     dx,ax
                mov     ah,2
                int     010h
                mov     ah,ch
                jmp     $fas2

        $revmode:
                mov     al,BYTE PTR _vidmode
                cmp     al,3
                je      $color3
                mov     ah,070h
                jmp     $fas2
        $color3:
                mov     ah,02eh
                jmp     $fas2           ;ready for next char

         $unrev:
                mov     al,BYTE PTR _vidmode
                cmp     al,3
                je      $col3
                mov     ah,07h
                jmp     $fas2
          $col3:
                mov     ah,0eh
                jmp     $fas2

         $replin:
                mov     dh,ah         ;save the video attribute in dh
                lodsb                 ;load number of lines in al
                push    si            ;save di and ds on stack
                push    ds
                mov     si,di
                sub     si,160        ;set di to point to start of last line
                mov     ds,_video_base
                mov     cl,80
                mul     cl            ;multiply lines by 80
                mov     cx,ax         ;put resulting count in cx for loop command
       $linloop:
                lodsw
                stosw
                loop    $linloop
                pop     ds
                pop     si
                mov     ah,dh
                jmp     $fas2

        $hilite:
                cmp     BYTE PTR _vidmode,3
                jne     $+5
                jmp     $fas2
                or      ah,080h
                jmp     $fas2

       $undline:
                cmp     BYTE PTR _vidmode,7
                je      $+5
                jmp     $fas2
                and     ah,0F8h
                or      ah,02h
                jmp     $fas2

         $thend:
                cld
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_fasrite        ENDP


; void ridofdowns(byte whatline)
; whatline = bp+4
;
;
        public  _ridofdowns
_ridofdowns     proc    near
        push    bp
        mov     bp,sp
        sub     sp,6     ;reserve space on the stack

        push    es
        push    si
        push    di

        mov     es,_video_base
        mov     al,byte ptr [bp+4]
        xor     ah,ah
        dec     ax
        xor     ch,ch
        mov     cl,160
        mul     cl
        mov     si,ax
        shr     cl,1

        push    ds
        mov     ax,es
        mov     ds,ax
ridagain:
        mov     di,si
        lodsb
        cmp     al,'Ë'
        je      m1
        cmp     al,'Ñ'
        je      m1
        cmp     al,'Ø'
        je      m2
        cmp     al,'Î'
        je      m3
        cmp     al,'Â'
        je      m4
        cmp     al,'Ò'
        je      m4
        cmp     al,'Å'
        je      m5
        cmp     al,'×'
        je      m6
ridgo:
        stosb
        inc     si
        loop    ridagain
        jmp     ridone
m1:
        mov     al,'Í'
        jmp     ridgo
m2:
        mov     al,'Ï'
        jmp     ridgo
m3:
        mov     al,'Ê'
        jmp     ridgo
m4:
        mov     al,'Ä'
        jmp     ridgo
m5:
        mov     al,'Á'
        jmp     ridgo
m6:
        mov     al,'Ð'
        jmp     ridgo

ridone:
        pop     ds
        pop     di
        pop     si
        pop     es

        mov     sp,bp
        pop     bp
        ret
_ridofdowns     endp

        PUBLIC  _invblk
_invblk         PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed
                mov     byte ptr [bp+9],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;start
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     cx,[bp+8]       ;stop
                sub     cx,[bp+6]       ;start
                inc     cx

                mov     es,_video_base  ;put base address in es for STOSB
                mov     ax,_inv_mask    ;put in attribute STOSB

          $blk1:
                and     es:[di],ah
                or      es:[di],al
                inc     di
                inc     di
                loop    $blk1

;|***
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_invblk         ENDP

        PUBLIC  _flash
_flash          PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;col
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     es,_video_base  ;put base address
                mov     al,80h

                or      es:[di],al

                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_flash          ENDP

        PUBLIC  _unflsh
_unflsh         PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;col
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     es,_video_base  ;put base address
                mov     al,7Fh

                and     es:[di],al
;|***
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_unflsh         ENDP

        PUBLIC  _att
_att            PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed
                mov     byte ptr [bp+9],0

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;col
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     es,_video_base  ;put base address
                mov     al,[bp+8]       ;attr

                mov     es:[di],al

                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_att            ENDP

        PUBLIC  _attcol
_attcol         PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed
                mov     byte ptr [bp+9],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+11],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+6] ;col
                mov     cx,[bp+4]       ;start
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     es,_video_base  ;put base address
                mov     al,[bp+8]       ;attr

                mov     cx,[bp+8]
                sub     cx,[bp+6]
                inc     cx

$nextcol:
                mov     al,es:[di]
                and     al,0f0h
                or      al,[bp+10]
                mov     es:[di],al
                add     di,160
                loop    $nextcol
;|***
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_attcol         ENDP

        PUBLIC  _attclr
_attclr         PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed
                mov     byte ptr [bp+9],0

                mov     ax,160
                mul     BYTE PTR [bp+4] ;ro
                mov     cx,[bp+6]       ;start
                shl     cx,1
                add     ax,cx
                sub     ax,161
                mov     di,ax           ;offs

                mov     cx,[bp+8]       ;stop
                sub     cx,[bp+6]       ;start
                inc     cx

                mov     es,_video_base  ;put base address in es for STOSB
                mov     ax,_clr_mask    ;put in attribute STOSB



          $clr1:
                and     es:[di],ah
                or      es:[di],al
                inc     di
                inc     di
                loop    $clr1


                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_attclr         ENDP

        PUBLIC  _scrncopy
_scrncopy       PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di
                push    ds

                push    ds
                pop     es
                mov     di,[bp+4]       ;scrn address for movs instruct
                mov     cx,2000         ;move 2000 words;
                mov     ds,_video_base  ;put base address in ds for movs
                sub     si,si           ;clear si for movs
                cld                     ;set direction flag to forward
                rep     movsw

                pop     ds
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_scrncopy       ENDP

        PUBLIC  _scrnrite
_scrnrite       PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     es,_video_base  ;put base address in es for movs
                sub     di,di           ;clear di for movs
                mov     si,[bp+4]       ;scrn address for movs instruct
                mov     cx,2000         ;move 0 to 1999 words;
                cld                     ;set direction flag to forward
                rep     movsw

                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_scrnrite       ENDP

        PUBLIC  _myclrscr
_myclrscr       PROC near
                push    bp
                mov     bp,sp
                push    si
                push    es
                push    di

                mov     byte ptr [bp+5],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+7],0  ; were passed
                mov     byte ptr [bp+9],0  ; zero out top half of parameter since bytes
                mov     byte ptr [bp+11],0  ; were passed

                mov     ax,160
                mul     BYTE PTR [bp+4] ;r1
                mov     cx,[bp+6]       ;c1
                shl     cx,1
                add     ax,cx
                sub     ax,162
                mov     di,ax           ;offset for stosw

                mov     es,_video_base  ;put base address in es for stosw

                mov     bx,[bp+10]      ;get C2
                sub     bx,[bp+6]       ;subtract C1 to get # of columns
                jl      exit            ;do nothing if columns diff is negative
                inc     bx

                mov     dx,bx
                shl     dx,1
                neg     dx
                add     dx,160          ;calculate amount added for each row

                mov     si,[bp+8]       ;get R2
                sub     si,[bp+4]       ;subtract R1 to get # of rows
                jl      exit            ;do nothing if rows diff is negative
                inc     si

                mov     ah,BYTE PTR _video_attr
                mov     al,20h

           arow:
                mov     cx,bx           ;move bx # of words;
                cld                     ;set direction flag to forward
                rep     stosw

                add     di,dx           ;start at next row down;

                dec     si
                or      si,si           ;set flags
                jg      arow

           exit:
                pop     di
                pop     es
                pop     si
                mov     sp,bp
                pop     bp
                ret

_myclrscr       ENDP

        PUBLIC  _gorocol
_gorocol        PROC NEAR
                push    bp
                mov     bp,sp

                mov     dh,BYTE PTR [bp+4]       ;ro
                dec     dh
                mov     dl,BYTE PTR [bp+6]       ;col
                dec     dl

                mov     ah,2
                mov     bh,0

                int     10h

                mov     sp,bp
                pop     bp
                ret

_gorocol        ENDP

        PUBLIC  _wherecur
_wherecur       PROC NEAR
                push    bp
                mov     bp,sp
                mov     ah,3
                mov     bh,0

                int     10h

                xor     ax,ax         ;zero out ax

                mov     al,dh

                mov     bx,[bp+4]       ;ro
                inc     al
                mov     [bx],al       ;ro

                xor     dh,dh         ;zero out dh

                mov     bx,[bp+6]       ;col
                inc     dx
                mov     [bx],dl       ;col

                mov     sp,bp
                pop     bp
                ret

_wherecur       ENDP


_TEXT   ENDS
END
