import r2pipe
import sys
import struct
import mmap
import os
import base64
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
#print funcs
#exit(1)

#open patch guide
guide_to_open=sys.argv[2]
with open(guide_to_open) as f: 
	content = f.readlines()
content = [x.strip() for x in content]
print content
patches = []
functions_to_hash = []
for c in content:
	s = c.split(',')
	target_func = 'sym.'+s[0]
	add_placeholder = int(s[1])
	size_placeholder = int(s[2])
	hash_placeholder = int(s[3])
	if target_func in funcs:
	#Compute expected hashes
		
		offset = funcs[target_func]['offset']
		size = funcs[target_func]['size']

		print 'Precomputing hash'
		h = 0
		print "p6e {}@{}".format(size,offset)
		b64_func = r2.cmd("p6e {}@{}".format(size,offset))
		func_bytes = bytearray(base64.b64decode(b64_func))
		#import binascii
		#print binascii.hexlify(func_bytes)
		#print list(func_bytes)
		for b in func_bytes:
			sys.stdout.write("%x "%b)
			h = h ^ b
		print ' '
		print target_func,' hash:',hex(h) 


		patch = {'add_placeholder':add_placeholder, 
			'size_placeholder':size_placeholder, 
			'hash_placeholder':hash_placeholder,
			'add_target' : offset,
			'size_target': size,
			'hash_target': h}
		patches.append(patch)
		#insert function to hash
		function_to_hash={'name':target_func,'address':offset, 'size':size}
		functions_to_hash.append(function_to_hash)
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
		###  PATCH expected hash ####
                addr = mm.find(struct.pack('<I',patch['hash_placeholder']))
                hash_patch = False
                if addr != -1: 
                        print 'Found a hash placeholder... attempting to patch'
                        mm.seek(addr,os.SEEK_SET)
                        patch_byte= struct.pack('<I',patch['hash_target'])
                        print 'patched {} with {} @ {}'.format(patch['hash_placeholder'],patch_byte.encode('hex'),addr)
                        mm.write(patch_byte)
                        hash_patch = True


		if not size_patch or not address_patch or not hash_patch:
			print 'Failed to find size and/or address and/or hash patches'
			exit(1) 


