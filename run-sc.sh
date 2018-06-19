
echo 'program.c input sc-include-func assert-skip-func-file'

UTILS_LIB=/home/sip/self-checksumming/build/lib/libUtils.so
INPUT_DEP_PATH=/usr/local/lib/
SC_PATH=/home/sip/self-checksumming/build/lib


#------------------ARGS for the script-------------
Source=$1		#Prgram to protect
FilterFile=$2		#List of sensitive functions, one name per line
Con=$3			#Connectivity level

if [ $Con = ""]; then
	Con=2
fi


#-------------------IMPORTANT Input Dependency Transformations --------
#-extract-functions
#-extraction-stats
#-extraction-stats-file="extract.stats"

#-------------------IMPORTANT CMD ARGS for SC------------------
#-connectivity= N		set the desired connectivity of the checkers network

#-extracted-only  		protect extracted only, extracted are never checkers, always checkees

#-use-other-functions		to meet the desired connectivity use other functions 
#				(not in the filter list) as checkers

#-dump-checkers-network		dump the network in the specified path

#-sensitive-only-checked	sensitive functions are never checkers but checkees, 
#				extracted only assumes this regardless of the flag  

#$1 is the .c file for transformation
echo 'build changes'
make -C build/

if [ $? -eq 0 ]; then
	    echo 'OK Compile'
    else
	        echo 'FAIL Compile'
	       exit	
	fi



clang-3.9 $Source -c -emit-llvm -o guarded.bc

echo 'Remove old files'
rm guide.txt
rm protected
rm out.bc
rm out

bitcode=guarded.bc

echo 'Transform SC'
opt-3.9 -load $INPUT_DEP_PATH/libInputDependency.so -load /usr/local/lib/libLLVMdg.so -load $UTILS_LIB -load $OH_LIB/liboblivious-hashing.so -load $INPUT_DEP_PATH/libTransforms.so -load $SC_PATH/libSCPass.so $bitcode -strip-debug -unreachableblockelim -globaldce -lib-config=/home/sip/input-dependency-analyzer/library_configs/tetris_library_config.json -extract-functions -sc -connectivity=$Con -dump-checkers-network="network_file" -dump-sc-stat="sc.stats" -filter-file=$FilterFile -o out.bc

if [ $? -eq 0 ]; then
	    echo 'OK Transform'
    else
	        echo 'FAIL Transform'
	       exit	
	fi



#link guardMe function
clang-3.9 rtlib.c -c -emit-llvm -o rtlib.bc
llvm-link-3.9 out.bc rtlib.bc -o out.bc


echo 'Post patching binary after hash calls'
llc-3.9 out.bc
gcc -c -rdynamic out.s -o out.o -lncurses
#gcc -g -rdynamic -c rtlib.c -o rtlib.o
gcc -g -rdynamic out.o response.o -o out -lncurses 

#clang++-3.9 -lncurses -rdynamic -std=c++0x out.bc -o out
python patcher/dump_pipe.py out guide.txt patch_guide
echo 'Done patching'

chmod +x out
echo 'Protected file: out'
