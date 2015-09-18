#
# Nana makefile for DOS/DJGPP
#
# Should work under the latest DJGPP.  Requires make, gcc, pmode13b, upx.
# My DJGPP directory is mounted as Y:\DJGPP in DOSBox; modify as needed
#

all: nana.exe

# Debug
nanad.exe: nana.c djgpp.c fileopen.c platform.h font.h
	gcc -O9 -Wall -fomit-frame-pointer nana.c djgpp.c fileopen.c -o $@

# Release
nana.exe: nanad.exe
	exe2coff nanad.exe
	strip nanad
	copy /b y:\djgpp\bin\pmodstub.exe+nanad nana.exe
	del nanad
	upx --best nana.exe

clean:
	del nanad.exe
	del nanad
	del nana.exe
