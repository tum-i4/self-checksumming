import r2pipe
import sys
import struct
import mmap
import os
class Patch:
	placeholder=''
	target=''

if len(sys.argv) != 3:
  print 'Incorrect usage. Expected: <binary_file_name>, <patch_guide_file_name>'
  sys.exit(1)
#open binary
file_to_open = sys.argv[1] 
r2 = r2pipe.open(file_to_open)
#find addresses and sizes of all functions
r2.cmd("aaa")
function_list = r2.cmdj("aflj")
funcs = {}
for function in function_list:
	attr = {'size':function['size'], 'offset':function['offset']}
	funcs[function['name']] = attr
	#print function
	#print function['name'], function['offset'], function['size']
#function has name, offset and size
print funcs
#exit(1)

#open patch guide
guide_to_open=sys.argv[2]
with open(guide_to_open) as f: 
	content = f.readlines()
content = [x.strip() for x in content]
print content
patches = []
for c in content:
	s = c.split(',')
	target_func = 'sym.'+s[0]
	add_placeholder = int(s[1])
	size_placeholder= int(s[2])
	if target_func in funcs:
		offset = funcs[target_func]['offset']
		size = funcs[target_func]['size']
		patch = {'add_placeholder':add_placeholder, 
			'size_placeholder':size_placeholder, 
			'add_target' : offset,
			'size_target': size}
		patches.append(patch)
#open hex editor

print patches 
with open(sys.argv[1], 'r+b') as f:
	mm = mmap.mmap(f.fileno(), 0)
	for patch in patches:
		###  PATCH address ####
		addr = mm.find(struct.pack('<I',patch['add_placeholder']))
		address_patch = False
		if addr != -1:
			print 'Found an address placeholder... attempting to patch'
			mm.seek(addr,os.SEEK_SET)
			patch_byte= struct.pack('<I',patch['add_target'])
			print 'patched {} with {} @ {}'.format(patch['add_placeholder'],patch_byte.encode('hex'),addr)
			mm.write(patch_byte)
			address_patch = True


		### PATCH size ####
		#mm.seek(0)
		size_patch = False
		print struct.pack('<H',patch['size_placeholder']).encode('hex')
		addr = mm.find(struct.pack('<H',patch['size_placeholder']))
		if addr != -1:
			print 'Found a size placeholder... attempting to patch'
			mm.seek(addr,os.SEEK_SET)
			patch_byte= struct.pack('<H',patch['size_target'])
			print 'patched {} with {} @ {}'.format(patch['size_placeholder'],patch_byte.encode('hex'),    addr)
			mm.write(patch_byte)
			size_patch = True
		if not size_patch or not address_patch:
			print 'Failed to find size and/or address patches'
			exit(1) 



#find placeholders for function addresses and sizes


#patch placeholders

#spit out patched binary

#r2 = r2pipe.open("/bin/ls")
#r2.cmd("aaa")
#function_list = r2.cmdj("aflj")
#print len(function_list)
#for function in function_list:
	#print r2.cmdj("p8j" + str(function["size"])+ " @ " + function["name"] )
#	print function
