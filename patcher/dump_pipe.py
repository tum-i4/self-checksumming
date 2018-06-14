import r2pipe
import sys
import struct
import mmap
import os
import base64
import os.path
import json
from pprint import pprint

debug_mode = False 
total_patches = 0
expected_patches = 0
def dump_debug_info(*args):
	global debug_mode
	if debug_mode:
		text = ""
		for arg in args:
			text+=arg
		print text
def precompute_hash(r2, offset, size):
		dump_debug_info( 'Precomputing hash')
		h = 0
		dump_debug_info( "p6e {}@{}".format(size,offset))
		b64_func = r2.cmd("p6e {}@{}".format(size,offset))
		func_bytes = bytearray(base64.b64decode(b64_func))
		for b in func_bytes:
			#sys.stdout.write("%x "%b)
			h = h ^ b
		dump_debug_info(  'hash:',hex(h)) 
		return h

def find_placeholder_sequential(mm,start_index,struct_flag, search_value):
        search_bytes = struct.pack(struct_flag, search_value);
	addr = mm.find(search_bytes,start_index)
	return addr

def find_placeholder(mm,struct_flag,search_value):
        search_bytes = struct.pack(struct_flag, search_value);
	addr = mm.find(search_bytes)
	if addr == -1:
		mm.seek(0)
	addr = mm.find(search_bytes)
	return addr

def patch_address(mm, addr, patch_value):
	mm.seek(addr,os.SEEK_SET)
	mm.write(patch_value)

def find_all_placeholders(mm,guard_patches):

    placeholder_addresses = {}
    for patch in guard_patches:
        for placeholder in patch:
            struct_flag = ''
            address = -1
            if placeholder == 'add_placeholder' or placeholder == 'hash_placeholder':
                struct_flag = '<I'
            elif placeholder == 'size_placeholder':
                struct_flag = '<I'
            if struct_flag!='':
                placeholder_value = patch[placeholder]
                address = find_placeholder(mm, struct_flag, placeholder_value)
                if placeholder_value not in placeholder_addresses:
                        placeholder_addresses[placeholder_value] = []   
                if address ==-1: 
                    dump_debug_info("ERR. Failed to find placeholder {} in the binary".format(placeholder_value))
                    exit(1)
                while address!=-1:
                    placeholder_addresses[placeholder_value].append(address)
                    start_index = address+1 
                    address = find_placeholder_sequential(mm, start_index, struct_flag, placeholder_value)
                
    found_addresses = 0
    pmap ={}
    for paddress in placeholder_addresses:
        count_placeholders = len(placeholder_addresses[paddress])
        for address in placeholder_addresses[paddress]:
            if address in pmap:
                pholder = pmap[address] 
                if pholder != paddress: 
                    print "ERR. Same address mapped to two placehoders",pholder, paddress 
                    exit(1)
        found_addresses =found_addresses + count_placeholders
    return placeholder_addresses



def patch_placeholder(mm, struct_flag,addresses, placeholder_value, target_value):
	global total_patches
	#addr = find_placeholder(mm,struct_flag,placeholder_value)
        if placeholder_value not in addresses:
            print "Err. can't find placeholder in the addresses"
        for addr in addresses[placeholder_value]:
            patch_bytes = struct.pack(struct_flag, target_value)
            patch_address(mm,addr,patch_bytes)
            dump_debug_info( 'Patched {} with {}'.format(placeholder_value, target_value))
            total_patches +=1

	return True

dump_mode = False	
if len(sys.argv) <3:
	print 'Incorrect usage. Expected: <binary_file_name>, <patch_guide_file_name>, [dump_computed_patches.json], optional:[sc.stats]'
	sys.exit(1)

if len(sys.argv) ==4:
	dump_debug_info( 'DUMP-MODE enabled, all patches are dumped into the specified dump_computed_patches.json')
	dump_mode = True
	patch_dump_file = sys.argv[3]
sc_stats={}
#We should not seek for patch guide when stats indicate no guards, see #42
if len(sys.argv) == 5:
    sc_stats = json.load(open(sys.argv[4]))
    if sc_stats["numberOfGuards"]==0:
        print 'SC stats indicates there is nothing to be patched'
        exit(0)
#open binary
file_to_open = sys.argv[1] 
r2 = r2pipe.open(file_to_open)
#find addresses and sizes of all functions
r2.cmd("aa")
function_list = r2.cmdj("aflj")
funcs = {}
for function in function_list:
	attr = {'size':function['size'], 'offset':function['offset']}
	funcs[function['name']] = attr

#open patch guide
guide_to_open=sys.argv[2]
if not os.path.exists(guide_to_open):
    print 'ERR. patch guide file cannot be found!'
    exit(1)
with open(guide_to_open) as f: 
	content = f.readlines()
	
content = [x.strip() for x in content]
dump_debug_info( 'conent: {}'.format( content))
patches = []
for c in content:
	s = c.split(',')
	target_func = s[0]
	add_placeholder = int(s[1])
	size_placeholder = int(s[2])
	hash_placeholder = int(s[3])
	if target_func not in funcs:
		target_func ='sym.'+target_func
	if target_func in funcs:
	#Compute expected hashes
		
		offset = funcs[target_func]['offset']
		size = funcs[target_func]['size']
		patch = {'add_placeholder':add_placeholder, 
			'size_placeholder':size_placeholder, 
			'hash_placeholder':hash_placeholder,
			'add_target' : offset,
			'size_target': size,
                        'hash_target': 0 , 'dummy': False}
		patches.append(patch)
	else:
		r2.cmd('s ' + target_func)
		funcinfo = r2.cmdj('afij')
		if funcinfo:
                        print 'TAKING ME'
                        print funcinfo
			func_info = funcinfo[0]
                        error = False
                        if target_func.replace('sym.','') != func_info['name'].replace('sym.',''):
                            print 'ERR. Target function {} does not match {}'.format(target_func, func_info['name'])
                            error = True
			offset = func_info['offset']
			size = func_info['size']
			patch = {'add_placeholder': add_placeholder,
					 'size_placeholder': size_placeholder,
					 'hash_placeholder': hash_placeholder,
					 'add_target': offset,
					 'size_target': size,
                                         'hash_target': 0, 'dummy':error}
			patches.append(patch)
		else:
			pprint (funcs)
			print 'ERR: failed to find function:{}'.format(target_func)
			exit(1)
if len(patches)!=len(content):
	print 'ERR: len (patches) != len( guide) {}!={}'.format(len(patches),len(content))
	exit(1)


#open hex editor
#every line containt information about 3 patches,
# size, address and hash that needs to be patched
expected_patches =len(patches)*3
dump_debug_info( "patches {}".format(patches) )
with open(sys.argv[1], 'r+b') as f:
	mm = mmap.mmap(f.fileno(), 0)
        
        #find addresses before starting to patch
        addresses = find_all_placeholders(mm, patches)


	dump_patch = []
	for patch in patches:
		address_patch = patch_placeholder(mm,'<I', addresses, patch['add_placeholder'], patch['add_target']) 
		if not address_patch:
			dump_debug_info( "can't patch address")
                size_target = patch['size_target']
                if patch['dummy']: 
                    size_target = 0
		size_patch = patch_placeholder(mm,'<I', addresses, patch['size_placeholder'], size_target)
		if not size_patch:
			dump_debug_info( "can't patch size")


		expected_hash = precompute_hash(r2, patch['add_target'], patch['size_target'])
                if patch['dummy']:
                    expected_hash = 0

		patch['hash_target'] = expected_hash
                hash_patch = patch_placeholder(mm,'<I', addresses, patch['hash_placeholder'],expected_hash) 
		if not hash_patch:
			dump_debug_info( "can't patch hash")


		if not size_patch or not address_patch or not hash_patch:
			print 'Failed to find size and/or address and/or hash patches'
			exit(1) 
		dump_patch.append(patch)
	dump_debug_info( 'expected patches:{}, total patched:{}'.format(expected_patches, total_patches))
	if total_patches != expected_patches:
		print 'Failed to patch all expected patches:',expected_patches, ' total patched:',total_patches
        else:
                print 'Successfuly patched all {} placeholders'.format(total_patches)
	if dump_mode ==True:
		import json
		with open(patch_dump_file, 'w') as outfile:
    			json.dump(dump_patch, outfile)


