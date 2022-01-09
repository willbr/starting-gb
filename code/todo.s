ld a *hl
xor *hl
ld a $ff
ld *$beef a
add hl bc
ld bc $cafe
ldh $FF $FF
ldi *hl a
ldd *hl a

