#Transform a given binary to be protected by OH and SC
#To add resilience, Input dependent instructions are extarcted
#and then protected by SC pass
#When possible SC guards are protected by OH guards.   
#If SC guards are called from input dependent branch, 
#then naturally OH cannot protect them.
#Process: Input indep func & inst-> OH ->
#         Input dep func -> SC 
#         Extract func   -> SC        


echo 'program.c input sc-include-func assert-skip-func-file'
ASSERT_SKIP_FILE=$4
SC_INCLUDE_FILE=$3




INPUT_DEP_PATH=/usr/local/lib/
SC_PATH=/home/sip/self-checksumming/build/lib

#$1 is the .c file for transformation
echo 'build changes'
make -C build/

clang-3.9 $1 -c -emit-llvm -o guarded.bc
#clang-3.9 rtlib.c -c -emit-llvm
echo 'Remove old files'
rm guide.txt
rm protected
rm out.bc
rm out

echo 'Transform SC'
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $SC_PATH/libSCPass.so guarded.bc -sc -input-dependent-functions=0 -functions="$SC_INCLUDE_FILE" -dump-checkers-network="$ASSERT_SKIP_FILE" -o guarded.bc


echo 'Set OH path configuration'
#INPUT_DEP_PATH=/usr/local/lib/
OH_PATH=/home/sip/sip-oblivious-hashing
OH_LIB=$OH_PATH/build/lib
bitcode=guarded.bc
input=$2



# extract input dependent functions pass
opt-3.9 -load $OH_PATH/lib/libInputDependency.so -load $OH_PATH/lib/ibTransforms.so bitcode.bc -extract-functions -o out.bc

# compiling external libraries to bitcodes
clang-3.9 $OH_PATH/assertions/response.c -c -fno-use-cxa-atexit -emit-llvm -o $OH_PATH/assertions/response.bc

# Running hash insertion pass
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load  $OH_LIB/liboblivious-hashing.so $bitcode -skip 'hash' -oh-insert -num-hash 1 -assert-functions=$ASSERT_SKIP_FILE -o out.bc

echo 'Post patching binary after hash calls'
llc-3.9 out.bc
gcc -c -rdynamic out.s -o out.o
# Linking with external libraries
gcc -g -rdynamic -c $OH_PATH/assertions/response.c -o response.o
gcc -g -rdynamic -c rtlib.c -o rtlib.o
gcc -g -rdynamic out.o response.o rtlib.o -o out 

#clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'

#Patch using GDB
python $OH_PATH/patcher/patchAsserts.py out out_patched

chmod +x out_patched
