wgb:
	watchexec -cr "make gb"

gb: src/main.c src/opcodes.h
	tcc -run $< ".\roms\tetris.gb"

src/opcodes.h: src/gen-opcodes.py
	python $< $@
	type "src\opcodes.h"

wop:
	watchexec -cr --filter "*.py" "make src/opcodes.h"

