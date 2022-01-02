wgb:
	watchexec -cr "make gb"

gb: src/main.c
	tcc -run $<

