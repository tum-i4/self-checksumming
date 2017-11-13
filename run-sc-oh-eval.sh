
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


bitcode=$1
filter_file=$2
clang-3.9 rtlib.c -c -emit-llvm -o rtlib.bc
llvm-link-3.9 $bitcode rtlib.bc -o guarded.bc

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



echo 'Transform SC & OH'
#opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so - -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load $OH_LIB/liboblivious-hashing.so $bitcode -sc -dump-checkers-network="$ASSERT_SKIP_FILE" -skip 'hash' -oh-insert -num-hash 1 -o out.bc
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load $OH_LIB/liboblivious-hashing.so -load $INPUT_DEP_PATH/libTransforms.so $bitcode -sc -connectivity=5  -dump-checkers-network="network_file" -dump-sc-stat="sc.stats" -filter-file=$filter_file -o out.bc 


if [ $? -eq 0 ]; then
	    echo 'OK Transform'
    else
	        echo 'FAIL Transform'
	       exit	
	fi


# compiling external libraries to bitcodes
clang-3.9 $OH_PATH/assertions/response.c -c -fno-use-cxa-atexit -emit-llvm -o $OH_PATH/assertions/response.bc
if [ $? -eq 0 ]; then
	    echo 'OK external'
    else
	        echo 'FAIL external'
	       exit	
	fi



echo 'Post patching binary after hash calls'
llc-3.9 out.bc

if [ $? -eq 0 ]; then
	    echo 'OK llc'
    else
	        echo 'FAIL llc'
	       exit	
	fi



gcc -c -rdynamic out.s -o out.o -lncurses
if [ $? -eq 0 ]; then
	    echo 'OK -c'
    else
	        echo 'FAIL -c'
	       exit	
	fi


# Linking with external libraries
gcc -g -rdynamic -c $OH_PATH/assertions/response.c -o response.o
if [ $? -eq 0 ]; then
	    echo 'OK -g1'
    else
	        echo 'FAIL -g1'
	       exit	
	fi


#gcc -g -rdynamic -c rtlib.c -o rtlib.o
gcc -g -rdynamic out.o response.o -o out -lncurses 
if [ $? -eq 0 ]; then
	    echo 'OK -g2'
    else
	        echo 'FAIL -g2'
	       exit	
	fi


#clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'
./out
#Patch using GDB
#python $OH_PATH/patcher/patchAsserts.py out out_patched

#chmod +x out_patched
