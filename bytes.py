import sys
BLOCK_SIZE = 1024
blockID = int(sys.argv[1])
form = "[{:0>4}]"
with open("VirtualMachine/SLOT_1/data.vhd", "rb") as file:
	file.seek(blockID * BLOCK_SIZE)
	buff = file.read(BLOCK_SIZE)
	for i in range(BLOCK_SIZE):
		if i % 32 == 0:
			print(form.format(i), end="")
		print("{:0>2}".format(hex(buff[i])[2: 4]), end=" ")
		if i % 32 == 31:
			print()
		elif i % 8 == 7:
			print(" ", end="")
