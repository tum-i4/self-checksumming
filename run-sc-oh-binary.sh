
echo 'program.c input sc-include-func assert-skip-func-file'

UTILS_LIB=/home/sip/self-checksumming/build/lib/libUtils.so
INPUT_DEP_PATH=/usr/local/lib/
SC_PATH=/home/sip/self-checksumming/build/lib

#$1 is the .c file for transformation
echo 'build changes'
make -C build/

if [ $? -eq 0 ]; then
	    echo 'OK Compile'
    else
	        echo 'FAIL Compile'
	       exit	
	fi



clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm -o rtlib.bc
llvm-link-3.9 guarded.bc rtlib.bc -o guarded.bc

echo 'Remove old files'
rm guide.txt
rm protected
rm out.bc
rm out

echo 'Set OH path configuration'
#INPUT_DEP_PATH=/usr/local/lib/
OH_PATH=/home/sip/sip-oblivious-hashing
OH_LIB=$OH_PATH/build/lib
bitcode=guarded.bc
input=$2


echo 'Transform SC & OH'
#opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so - -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load $OH_LIB/liboblivious-hashing.so $bitcode -sc -dump-checkers-network="$ASSERT_SKIP_FILE" -skip 'hash' -oh-insert -num-hash 1 -o out.bc
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load $OH_LIB/liboblivious-hashing.so -load $INPUT_DEP_PATH/libTransforms.so $bitcode -clone-functions -extract-functions -sc -connectivity=1  -dump-checkers-network="network_file" -oh-insert -num-hash 1 -o out.bc

#-load-checkers-network="intresting_network"
#-dump-checkers-network="network_file"


# compiling external libraries to bitcodes
clang-3.9 $OH_PATH/assertions/response.c -c -fno-use-cxa-atexit -emit-llvm -o $OH_PATH/assertions/response.bc

# Running hash insertion pass
#opt-3.9 -load $UTILS_LIB -load $INPUT_DEP_PATH/libInputDependency.so -load  $OH_LIB/liboblivious-hashing.so $bitcode -skip 'hash' -oh-insert -num-hash 1 -o out.bc

echo 'Post patching binary after hash calls'
llc-3.9 out.bc
gcc -c -rdynamic out.s -o out.o
# Linking with external libraries
gcc -g -rdynamic -c $OH_PATH/assertions/response.c -o response.o
#gcc -g -rdynamic -c rtlib.c -o rtlib.o
gcc -g -rdynamic out.o response.o -o out 

#clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'

#Patch using GDB
python $OH_PATH/patcher/patchAsserts.py out out_patched

chmod +x out_patched
