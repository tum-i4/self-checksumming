INPUT_DEP_PATH=/usr/local/lib
SC_PATH=/home/sip/self-checksumming/build/lib
UTILS_LIB=/home/sip/self-checksumming/build/lib/libUtils.so
#$1 is the .c file for transformation
#$2 only protect input dependent functions
echo 'build changes'
make -C build/

clang-3.9 $1 -c -emit-llvm -o guarded.bc
clang-3.9 rtlib.c -c -emit-llvm
echo 'Remove old patch guide file'
rm guide.txt
echo 'Transform'
#opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $SC_PATH/libSCPass.so guarded.bc -sc -input-dependent-functions=$2 -dump-checkers-network="checkers.json" -functions="sc-include" -o guarded.bc
#opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $UTILS_LIB -load $SC_PATH/libSCPass.so guarded.bc -sc -dump-checkers-network="checkers.json" -o guarded.bc
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load $UTILS_LIB -load $SC_PATH/libSCPass.so -load $INPUT_DEP_PATH/libTransforms.so guarded.bc -lib-config=/home/sip/input-dependency-analyzer/library_configs/tetris_library_config.json -extract-functions -sc -connectivity=2 -maximum-input-independent-percentage=100 -dump-checkers-network="network_file" -dump-sc-stat="sc.stats" -filter-file="" -dump-oh-stat="oh.stats" -extraction-stats -extraction-stats-file="extract.stats" -dependency-stats -dependency-stats-file="dependency.stats" -o out.bc
echo 'Link'
llvm-link-3.9 out.bc rtlib.bc -o out.bc
echo 'Binary'
clang-3.9 out.bc -o out 

echo 'Post patching'
python patcher/dump_pipe.py out guide.txt patch_guide

echo 'Run'
./out

