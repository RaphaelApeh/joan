CFLAGS = -Wall -Wextra -Werror -L../build -l:libjoan.dll.a -I../include
EXE = -o test.exe && .\test.exe
# Windows
test-win:
	gcc test.c ${CFLAGS} ${EXE}
# Linux or Macos
test:
	gcc test.c -Wall -Wextra -Werror -L../build -l:libjoan.a -I../include -o test && ./test

test_buff:
	gcc test_buff.c ${CFLAGS} ${EXE}

test_vm:
	gcc test_vm.c ${CFLAGS} ${EXE}

clean-win:
	del test.exe

clean:
	rm -f test