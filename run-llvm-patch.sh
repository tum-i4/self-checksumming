
#$1 is the .c file for transformation
#$2 is dump patch json file
echo 'build changes'
cd build
make
cd ..

clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm
echo 'Remove old patch guide file'
rm guide.txt
echo 'Transform'
opt-3.9 -load build/src/libSCPass.so guarded.bc -sc -o guarded.bc
echo 'Link'
llvm-link-3.9 guarded.bc rtlib.bc -o out.bc
echo 'Binary'
clang-3.9 out.bc -o out 

echo 'Post patching'
python patcher/dump_pipe.py out guide.txt $2

echo 'remove post patched binary to make sure llvm is patching'
rm out 

echo 'Patch in llvm'
opt-3.9 -load build/src/libSCPatchPass.so out.bc -scpatch -o out.bc
clang-3.9 out.bc -o out

echo 'Run'
./out
#clang -Xclang -load -Xclang build/src/libSCPass.so -c $1 -o guarded.o
#cc -c rtlib.c
#cc guarded.o rtlib.o
#./a.out

