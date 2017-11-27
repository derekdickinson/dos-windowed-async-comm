COM1      equ    1

if COM1

COMBASE   equ    3F8h
TXHOLD    equ    3F8h
RXDATA    equ    3F8h
BAUDLOW   equ    3F8h
BAUDHIGH  equ    3F9h
INTENA    equ    3F9h
INTIDREG  equ    3FAh
LINECTRL  equ    3FBh
MDMCTRL   equ    3FCh
LINESTUS  equ    3FDh
MDMSTUS   equ    3FEh
COM_INT   equ    0Ch

else

COMBASE   equ    2F8h
TXHOLD    equ    2F8h
RXDATA    equ    2F8h
BAUDLOW   equ    2F8h
BAUDHIGH  equ    2F9h
INTENA    equ    2F9h
INTIDREG  equ    2FAh
LINECTRL  equ    2FBh
MDMCTRL   equ    2FCh
LINESTUS  equ    2FDh
MDMSTUS   equ    2FEh
COM_INT   equ    0bh

endif

STARTSEND equ    2

DATAREADY equ    8

STX       equ    2
ETX       equ    3
DLE       equ    10h

PAD       equ    0FFH

CTRLSEND  equ    26

PADMAX    equ    4
NUMPADS   equ    4
WHENCRC   equ    4

; The below states apply to the "_txstate" variable.
;
; Standard Send states.
SENDIDLE  equ    0
SENDSTRT  equ    2
SENDSTX   equ    4
SENDSEQ   equ    6
SENDLEN0  equ    8
SENDLEN1  equ   10
SENDATA   equ   12
SENDETX   equ   14
SENDCRC0  equ   16
SENDCRC1  equ   18
PACKDONE  equ   20

; These apply to the the "_rxstate" variable.
;
; Standard Receive states.
;
RCVIDLE   equ    0
RCVSTRT   equ    2
RCVSTX    equ    4
RCVSEQ    equ    6
RCVLEN0   equ    8
RCVLEN1   equ   10
RCVDATA   equ   12
RCVETX    equ   14
RCVCRC0   equ   16
RCVCRC1   equ   18
RCVDONE   equ   20
;

BIGSTR  STRUC
len     dw   ?
crc     dw   ?
buf     db   1024 dup(?)
BIGSTR  ENDS

LITSTR  STRUC
lens    dw   ?
crcs    dw   ?
sts     db   250 dup(?)
LITSTR  ENDS