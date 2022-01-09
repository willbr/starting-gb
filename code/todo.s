ld a *hl
xor *hl
ld a $ff
ld *$beef a
add hl bc
ld bc $cafe
ld a $FF
ldh $FF a
ldi *hl a
ldd *hl a

