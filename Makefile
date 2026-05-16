CL=	wcl
CLFLAGS= -bt=dos -mc
#WATCOM= /path/to/watcom
INCLUDES= -i=$(WATCOM)\h

all: main.exe rmalpha.exe

main.exe: main.c
	$(CL) $(CLFLAGS) $(INCLUDES) main.c

rmalpha.exe: rmalpha.c
	$(CL) $(CLFLAGS) $(INCLUDES) rmalpha.c

clean: .SYMBOLIC
	rm -f main.exe main.o rmalpha.exe rmalpha.o
