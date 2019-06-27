with open("VirtualMachine/SLOT_2/data.vhd", "wb") as file:
	for i in range(1001):
		file.write(b"0" * 1024)
