INPUT_DEP_PATH=/usr/local/lib/
SC_PATH=/home/sip/self-checksumming/build/lib

#$1 is the .c file for transformation
echo 'build changes'
make -C build/

clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm
echo 'Remove old files'
rm guide.txt
rm protected
rm out.bc
rm out

echo 'Transform SC'
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $SC_PATH/libSCPass.so guarded.bc -sc -input-dependent-functions=0 -o guarded.bc
#echo 'Link'
#llvm-link-3.9 guarded.bc rtlib.bc -o out.bc
#echo 'Binary'
#clang-3.9 out.bc -o out 


echo 'Set OH path configuration'
#INPUT_DEP_PATH=/usr/local/lib/
OH_PATH=/home/sip/sip-oblivious-hashing
OH_LIB=$OH_PATH/build/lib
bitcode=guarded.bc
input=$2

# compiling external libraries to bitcodes
clang++-3.9 $OH_PATH/assertions/asserts.cpp -fno-use-cxa-atexit -std=c++0x -c -emit-llvm -o $OH_PATH/assertions/asserts.bc
clang-3.9 $OH_PATH/hashes/hash.c -c -fno-use-cxa-atexit -emit-llvm -o $OH_PATH/hashes/hash.bc
clang++-3.9 $OH_PATH/assertions/logs.cpp -fno-use-cxa-atexit -std=c++0x -c -emit-llvm -o $OH_PATH/assertions/logs.bc

# Running hash insertion pass
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load  $OH_LIB/liboblivious-hashing.so $bitcode -skip 'hash' -oh-insert -num-hash 1 -assert-functions 'assert-func' -o out.bc
# Linking with external libraries
llvm-link-3.9 out.bc $OH_PATH/hashes/hash.bc -o out.bc
llvm-link-3.9 out.bc $OH_PATH/assertions/asserts.bc -o out.bc
llvm-link-3.9 out.bc $OH_PATH/assertions/logs.bc -o out.bc
llvm-link-3.9 out.bc rtlib.bc -o out.bc

echo 'Post patching binary after hash calls'
clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'

#Run to compute expected oh hashes
echo 'Precompute intermediate hashes'
./out $input

# Running assertion insertion pass
rm hashes_dumper.log
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $OH_LIB/liboblivious-hashing.so out.bc -insert-asserts -o out.bc


#Write binary for hash, address and size computation
clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out


echo 'Post patching binary after assert calls'
python patcher/dump_pipe.py out guide.txt patch_guide

echo 'Run to compute final hashes'
./out $input 


#echo 'Patch in llvm'
#opt-3.9 -load build/lib/libSCPatchPass.so out.bc -scpatch -o outpatched.bc
#If patched here, all placeholders are gone, no way to patch after the asserts are finalized

llvm-dis-3.9 out.bc
echo 'Manually patch out.ll using the ids in hashes_dumper.log'

exit 1

#Runnig assertion finalization pass
#opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $OH_LIB/liboblivious-hashing.so out.bc -insert-asserts-finalize -o protected.bc
# Compiling to final protected binary
#clang++-3.9 -lncurses -rdynamic -std=c++0x protected.bc -o protected


#echo 'Running final protected binary'
python patcher/dump_pipe.py protected guide.txt 
./protected $input



#echo 'Patch in llvm'
#opt-3.9 -load build/lib/libSCPatchPass.so protected.bc -scpatch -o protected.bc


# precompute hashes
#clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
#should dump seg fault
#./out $input
###rm out
#


# Compiling to final protected binary
#clang++-3.9 -lncurses -rdynamic -std=c++0x protected.bc -o protected


#echo 'Run'
#./protected

