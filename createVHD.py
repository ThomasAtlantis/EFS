with open("VirtualMachine/SLOT_1/data.vhd", "wb") as file:
	for i in range(8193):
		file.write(bytes(1024))
