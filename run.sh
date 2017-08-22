
#$1 is the .c file for transformation


clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm
echo 'Transform'
opt-3.9 -load build/src/libSCPass.so guarded.bc -sc -o guarded.bc
echo 'Link'
llvm-link-3.9 guarded.bc rtlib.bc -o out.bc
echo 'Binary'
clang-3.9 out.bc -o out 
./out
#clang -Xclang -load -Xclang build/src/libSCPass.so -c $1 -o guarded.o
#cc -c rtlib.c
#cc guarded.o rtlib.o
#./a.out

