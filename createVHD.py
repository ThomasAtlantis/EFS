vhd_list = [
	"VirtualMachine/SLOT_1/data.vhd",
	"VirtualMachine/SLOT_2/data.vhd",
	"VirtualMachine/SLOT_X/USB_1/data.vhd",
]
for vhd in vhd_list:
	with open(vhd, "wb") as file:
		for i in range(8193):
			file.write(bytes(1024))
