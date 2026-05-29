src = ../rval.c ../cmd/cmd.c ../cmd/cmd_str.c ../cmd/cmd_dbg.c -lm
flags = -Wall

all:
	gcc -o a.exe storage.c $(src) $(flags)
	./a.exe

	gcc -o b.exe remote.c $(src) $(flags)
	./b.exe

storage:
	gcc -o a.exe storage.c $(src) $(flags)
	./a.exe

remote:
	gcc -o b.exe remote.c $(src) $(flags)
	./b.exe

main:
	gcc -o a.exe main.c $(src) $(flags)
	./a.exe