;
; Receive macros.
;
;
strt_in  MACRO
         LOCAL   not_dle

         cmp     al,DLE
         jne     not_dle
         mov     _rxstate,RCVSTX
not_dle:
         ENDM

stx_in   MACRO
         LOCAL   not_stx

         cmp     al,STX
         jne     not_stx
         mov     rxindex,0
         mov     _rxstate,RCVSEQ
         jmp     theloop
not_stx:
         mov     _rxstate,RCVSTRT
         ENDM

seq_in   MACRO
         LOCAL   notfrst

         mov     _rcvack,al
         and     _rcvack,0fh
         shrm    al,4
         mov     byte ptr rcvseq[0],al
         cmp     _firstime,0
         je      notfrst

         mov     _firstime,0
         mov     byte ptr _rcvtail[0],al
         sub     al,1
         and     al,0fh
         mov     byte ptr _rcvdata[0],al
         add     al,9
         and     al,0fh
         mov     byte ptr _rcvhead[0],al
notfrst:
         mov     _rxstate,RCVLEN0
         ENDM

len_in0  MACRO
         mov     byte ptr rcvlen[0],al
         mov     _rxstate,RCVLEN1
         ENDM

len_in1  MACRO
         LOCAL   notack,inrange

         mov     byte ptr rcvlen[1],al
         mov     chkcrc,0

         cmp     rcvlen,0
         jne     notack
         mov     _rxstate,RCVSTRT
         jmp     theloop

notack:
         cmp     rcvlen,1024
         jle     inrange
         mov     _rxstate,RCVSTRT
         jmp     theloop
inrange:
         setbufptr
         mov     _rxstate,RCVDATA
         ENDM


data_in  MACRO
         LOCAL   data_done, isfil,notdle

         mov     bx,_inbuf

         mov     ah,0
         add     chkcrc,ax

         mov     di,rxindex
         mov     byte ptr [bx][di],al

         inc     di
         mov     rxindex,di

         cmp     di,rcvlen
         jge     data_done

         jmp     theloop

data_done:
         mov     rxindex,0
         mov     _rxstate,RCVETX
         ENDM


etx_in   MACRO
         LOCAL   badframe

         cmp     al,ETX          ; Check for incorrect framing
         jne     badframe

         mov     si,rcvseq           ; Frame was good so store length
         shl     si,1
         mov     dx,rcvlen
         mov     _rcvlens[si],dx

         mov     _rxstate,RCVCRC0
         jmp     theloop

badframe:
         mov     si,rcvseq
         inc     _framerr[si]
         mov     _rxstate,RCVSTRT
         ENDM


crc0_in  MACRO
         mov     byte ptr rcvcrc[0],al
         mov     _rxstate,RCVCRC1
         ENDM


crc1_in  MACRO
         LOCAL   notstxal, notstxah, notbadcrc,noadvance

         mov     byte ptr rcvcrc[1],al

         mov     ax,chkcrc

         cmp     ax,rcvcrc
         je      notbadcrc

         mov     si,rcvseq
         inc     _crcerrs[si]
         mov     _rxstate,RCVSTRT
         jmp     theloop

notbadcrc:
         lea     ax,_dummybuf
         cmp     _inbuf,ax
         je      noadvance
         advance_rcv
noadvance:
         mov     _rxstate,RCVSTRT
         ENDM

advance_rcv MACRO
         LOCAL   whilestart,whilend,headone,headstrt,nochange

         mov     si,rcvseq
         mov     _arrived[si],1
         mov     si,_rcvtail

whilestart:
         cmp     _arrived[si],1
         jne     whilend

         mov     _arrived[si],0
         inc     si
         and     si,0fh
         jmp     whilestart
whilend:
         cmp     _rcvtail,si
         je      nochange

         mov     _rcvtail,si
         mov     di,si
         add     si,8
         and     si,0fh

;         putcon
; This loop will stop data from being overwritten before it is saved on disk.
headstrt:
         between di,si,_rcvdata
         jg      headone
         dec     si
         and     si,0fh
         jmp     headstrt
headone:
         mov     _rcvhead,si
nochange:

         mov     _startimer,1
         ENDM

setbufptr MACRO
         LOCAL   inwind,bufempt,done

         mov     si,rcvseq

         cmp     _arrived[si],0
         je      bufempt
         lea     ax,_dummybuf
         mov     _inbuf,ax
         jmp     done

bufempt:
         between _rcvtail,rcvseq,_rcvhead
         jg      inwind
         lea     ax,_dummybuf
         mov     _inbuf,ax
         jmp     done

inwind:
         mov     ax,rcvseq
         shlm    al,2
         lea     dx,_inbufs
         add     ax,dx
         mov     _inbuf,ax
done:
         ENDM


between  MACRO   tail,mid,head
         LOCAL   false1,false2,false3
; The between evaluation can be broken up into a three part conditional.
; If two or more are true then the number is between.

         mov     cx,0

         mov     ax,tail
         cmp     ax,mid
         jg      false1
         inc     cx
false1:
         mov     ax,mid
         cmp     ax,head
         jge     false2
         inc     cx
false2:
         mov     ax,head
         cmp     ax,tail
         jge     false3
         inc     cx
false3:
         cmp     cx,1
         ENDM