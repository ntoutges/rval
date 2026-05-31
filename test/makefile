src = ../rval.c ../cmd/cmd.c ../cmd/cmd_str.c ../cmd/cmd_dbg.c -lm
flags = -Wall

all:
	gcc -o a.exe storage.c $(src) $(flags)
	gcc -o b.exe remote.c $(src) $(flags)
	gcc -o c.exe local.c $(src) $(flags)

	./a.exe
	./b.exe
	./c.exe

storage:
	gcc -o a.exe storage.c $(src) $(flags)
	./a.exe

remote:
	gcc -o b.exe remote.c $(src) $(flags)
	./b.exe

local:
	gcc -o c.exe local.c $(src) $(flags)
	./c.exe

main:
	gcc -o a.exe main.c $(src) $(flags)
	./a.exe