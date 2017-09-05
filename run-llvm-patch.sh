sh run-binary-patch.sh $1 $2 $3
echo 'remove post patched binary to make sure llvm is patching'
rm out 

echo 'Patch in llvm'
opt-3.9 -load build/lib/libSCPatchPass.so out.bc -scpatch -o out.bc
clang-3.9 out.bc -o out

echo 'Run'
./out

