# -fno-finite-math-only to avoid GLIBC_2.15 requirement
FLAGS=-m32 -Os -ffast-math -fno-finite-math-only -fPIC -shared -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--no-undefined -flto
LIBS=-lm

gamei386.so: *.c *.h acesrc/* navlib/*.o makefile
	$(CC) $(FLAGS) *.c acesrc/*.c navlib/*.o $(LIBS) -o $@
	
clean:
	rm -f gamei386.so
