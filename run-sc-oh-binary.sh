
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
opt-6.0 -load $INPUT_DEP_PATH/libInputDependency.so -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load /usr/local/lib/libLLVMdg.so -load $OH_LIB/liboblivious-hashing.so -load $INPUT_DEP_PATH/libTransforms.so $bitcode -strip-debug -unreachableblockelim -globaldce -lib-config=/home/sip/input-dependency-analyzer/library_configs/tetris_library_config.json -extract-functions -sc -connectivity=2 -maximum-input-independent-percentage=100 -dump-checkers-network="network_file" -dump-sc-stat="sc.stats" -filter-file="" -oh-insert -short-range-oh -num-hash 1 -dump-oh-stat="oh.stats" -extraction-stats -extraction-stats-file="extract.stats" -dependency-stats -dependency-stats-file="dependency.stats" -o out.bc

if [ $? -eq 0 ]; then
	    echo 'OK Transform'
    else
	        echo 'FAIL Transform'
	       exit	
	fi

echo 'Post patching binary after hash calls'
llc-6.0 out.bc
g++ -std=c++0x -c -rdynamic out.s -o out.o

LIB_FILE=()
g++ -fPIC -std=c++11 -g -rdynamic -c ${OH_PATH}/assertions/response.cpp -o oh_rtlib.o
LIB_FILES+=( "${OH_PATH}/assertions/oh_rtlib.o" )

gcc -fPIC -g -rdynamic -c "${PWD}/rtlib.c" -o "${PWD}/sc_rtlib.o"
LIB_FILES+=( "${PWD}/sc_rtlib.o" )

g++ -std=c++11 -g -rdynamic -Wall -fPIC -shared -Wl,-soname,libsrtlib.so -o "librtlib.so" -lncurses -lm -lssl -lcrypto -pthread ${LIB_FILES[@]}

g++ -std=c++11 -g -rdynamic out.o -o out -L. -lrtlib -lncurses -lm -lssl -lcrypto -pthread

python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'

#Patch using GDB
export LD_PRELOAD="/home/sip/self-checksumming/hook/build/libminm.so ${PWD}/librtlib.so" 
args=' < /home/anahitik/SIP/sip-eval/my_cmdline-args/rawcaudio_small.pcm > output_small.adpcm'
python $OH_PATH/patcher/patchAsserts.py -b out -n out_patched -s oh.stats -g "$args" -p "${PWD}/gdb_script_for_do_assert.txt"
chmod +x out_patched
