ld hl $dead
ld a $ff
ld *hl a
xor *hl
ld a $ff
ld *$beef a
ld bc $cafe
ld de $babe
add hl bc
ld a $FF
ldh $FF a
ldi *hl a
ld a *hl
ldd *hl a
ld a *hl

