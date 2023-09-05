#cd bin/
#./c_compiler -S compiler_tests/_example/example.c  -o hi.txt
make
./bin/c_compiler -S compiler_tests/_example/example.c -o hi.txt
