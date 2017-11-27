
_DATA   SEGMENT
rcvlen  dw  0
rcvseq  dw  0
where   dw  0
chkcrc  dw  0
rcvcrc  dw  0
sendcrc dw  0
rxindex dw  0
txindex dw  0

intjmp  dw  mdmint,   txempt,   rxchar,   rcpint

mdmjmp  dw  badm0000
        dw  was_cts,  was_dsr,  badm0011, was_ri
        dw  badm0101, badm0110, badm0111, was_dcd
        dw  badm1001, badm1010, badm1011, badm1100
        dw  badm1101, badm1110, badm1111

txjmp   dw  tx_idle
        dw  tx_strt,  tx_stx,   tx_seq,   tx_len0
        dw  tx_len1,  tx_data,  tx_etx,   tx_crc0
        dw  tx_crc1,  tx_idle

rxjmp   dw  rx_idle
        dw  rx_strt,  rx_stx,   rx_seq,   rx_len0
        dw  rx_len1,  rx_data,  rx_etx,   rx_crc0
        dw  rx_crc1,  rx_idle

rcpjmp  dw  badr0000
        dw  was_over, was_party, badr0011, was_frerr
        dw  badr0101, badr0110,  badr0111, was_break
        dw  badr1001, badr1010,  badr1011, badr1100
        dw  badr1101, badr1110,  badr1111

_DATA   ENDS
