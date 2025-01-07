obj-m := message_slot.obj
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -c $(KDIR) M=$(shell pwd) modules
clean:
	$(MAKE) -c $(KDIR) M=$(shell pwd) clean