
#$1 is the .c file for transformation
echo 'build changes'
cd build
make
cd ..

clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm
echo 'Remove old files'
rm guide.txt
rm protected
rm out.bc
rm out

echo 'Transform SC'
opt-3.9 -load build/src/libSCPass.so guarded.bc -sc -o guarded.bc
echo 'Link'
#llvm-link-3.9 guarded.bc rtlib.bc -o out.bc
#echo 'Binary'
#clang-3.9 out.bc -o out 


echo 'Set OH path configuration'
INPUT_DEP_PATH=/usr/local/lib/
OH_PATH=/home/sip/sip-oblivious-hashing
OH_LIB=$OH_PATH/build/lib
bitcode=guarded.bc
input=$2

# compiling external libraries to bitcodes
clang++-3.9 $OH_PATH/assertions/asserts.cpp -fno-use-cxa-atexit -std=c++0x -c -emit-llvm -o $OH_PATH/assertions/asserts.bc
clang-3.9 $OH_PATH/hashes/hash.c -c -fno-use-cxa-atexit -emit-llvm -o $OH_PATH/hashes/hash.bc
clang++-3.9 $OH_PATH/assertions/logs.cpp -fno-use-cxa-atexit -std=c++0x -c -emit-llvm -o $OH_PATH/assertions/logs.bc

# Running hash insertion pass
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load  $OH_LIB/liboblivious-hashing.so $bitcode -oh-insert -num-hash 1 -o out.bc
# Linking with external libraries
llvm-link-3.9 out.bc $OH_PATH/hashes/hash.bc -o out.bc
llvm-link-3.9 out.bc $OH_PATH/assertions/asserts.bc -o out.bc
llvm-link-3.9 out.bc $OH_PATH/assertions/logs.bc -o out.bc
llvm-link-3.9 out.bc rtlib.bc -o out.bc

# precompute hashes
clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
#should dump seg fault
#./out $input
###rm out
#

# Running assertion insertion pass
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $OH_LIB/liboblivious-hashing.so out.bc -insert-asserts -o protected.bc
# Compiling to final protected binary
clang++-3.9 -lncurses -rdynamic -std=c++0x protected.bc -o protected




echo 'Post patching'
python patcher/dump_pipe.py protected guide.txt

echo 'Run'
./protected

