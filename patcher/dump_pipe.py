import r2pipe

#open binary

#find addresses and sizes of all functions

#open hex editor

#find placeholders for function addresses and sizes

#patch placeholders

#spit out patched binary

r2 = r2pipe.open("/bin/ls")
r2.cmd("aaa")
function_list = r2.cmdj("aflj")
print len(function_list)
for function in function_list:
	#print r2.cmdj("p8j" + str(function["size"])+ " @ " + function["name"] )
	print function
