default:
	cd src; make; cd ..

static:
	cd src; make static; cd ..

test_hex:
	cd src; make test_hex; cd ..

test_pic_hex:
	cd src; make test_pic_hex; cd ..

suid-install:
	chown root:root usb_pickit
	chmod u+s usb_pickit
	cp -p usb_pickit /usr/local/bin

install:
	cp usb_pickit /usr/local/bin

clean:
	cd src; make clean; cd ..
	rm -f \#* *.o core.* *~ .*~
