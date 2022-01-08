ld a *hl
xor *hl
ld *$beef $ff
add hl bc
ld bc $cafe
ldh $FF $FF
ldi *hl a
ldd *hl a

