src = ../cmd.c ../cmd_dbg.c ../cmd_str.c
flags = -Wall

all:
	gcc -o a.exe main.c $(src) $(flags)
	gcc -o b.exe long.c $(src) $(flags) -D'cmd_bbuf_ptr_t=uint16_t'
	gcc -o c.exe tx.c $(src) $(flags)
	gcc -o d.exe ftoa.c $(src) $(flags)
	gcc -o e.exe itoa.c $(src) $(flags)
	gcc -o f.exe atof.c $(src) $(flags)
	gcc -o g.exe atoi.c $(src) $(flags)
	gcc -o h.exe esc.c $(src) $(flags)

	./a.exe
	./b.exe
	./c.exe
	./d.exe
	./e.exe
	./f.exe
	./g.exe
	./h.exe

test:
	gcc -o a.exe main.c $(src) $(flags)
	./a.exe

long:
	gcc -o b.exe long.c $(src) $(flags) -D'cmd_bbuf_ptr_t=uint16_t'
	./b.exe

tx:
	gcc -o c.exe tx.c $(src) $(flags)
	./c.exe

ftoa:
	gcc -o d.exe ftoa.c $(src) $(flags)
	./d.exe

itoa:
	gcc -o e.exe itoa.c $(src) $(flags)
	./e.exe

atof:
	gcc -o f.exe atof.c $(src) $(flags)
	./f.exe

atoi:
	gcc -o g.exe atoi.c $(src) $(flags)
	./g.exe

esc:
	gcc -o h.exe esc.c $(src) $(flags)
	./h.exe