src = ../rval.c ../cmd/cmd.c ../cmd/cmd_str.c ../cmd/cmd_dbg.c -lm
flags = -Wall

storage:
	gcc -o a.exe storage.c $(src) $(flags)
	./a.exe

main:
	gcc -o a.exe main.c $(src) $(flags)
	./a.exe