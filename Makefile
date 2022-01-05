all : easido.exe

HEADER=strbuffer.h ealib.h
SOURCE=easido.cc strbuffer.cc ealib.cc

easido.exe : $(SOURCE:.cc=.o)
	gcc $^ -o $@ -Zcrtdll -lwrap

.cc.o :
	gcc -O2 -c $< -o $@

package :
	lxlite easido.exe
	lha a easido$(VER).lzh easido.txt easido.exe Makefile $(SOURCE) $(HEADER)

clean :
	rm easido.exe

install:
	cp easido.exe $(HOME)/bin/.
