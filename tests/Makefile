# Windows
test-win:
	gcc test.c -g -Wall -Wextra -Werror -L../build -l:libjoan.dll.a -I../include -o test.exe && .\test.exe
# Linux or Macos
test:
	gcc test.c -Wall -Wextra -Werror -L../build -l:libjoan.a -I../include -o test && ./test

test_buff:
	gcc test_buff.c -g -Wall -Wextra -Werror -L../build -l:libjoan.dll.a -I../include -o test.exe && .\test.exe

test_vm:
	gcc test_vm.c -g -Wall -Wextra -Werror -L../build -l:libjoan.dll.a -I../include -o test.exe && .\test.exe

clean-win:
	del test.exe

clean:
	rm -f test