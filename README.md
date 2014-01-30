qibutton
========

QIButton: read Dallas iButton temperature logger DS1922 with DS9490

About
=====
This tool can be used to read an iButton temperature sensor DS1922L 
via the USB 1-Wire adapter DS9490 (chip is a DS2490) on Linux. 
Two tools are included, a CLI and a GUI based on QT4. 

Requirements
============
* USB communication uses libusb. This only works if the kernel 
module ds2490 is not loaded (-> rmmod ds2490 or blacklist).
* The user needs r/w access to the USB device. A udev rule should
be used to change group to plugdev for Vendor ID 04FA, ProductID 2490
* QT4


TODO
====
- [ ] GUI is not yet finished
- [ ] Maybe: Write access via CLI
- [ ] Currently no ROM matching is done, so this works only for one 
1-Wire device on the bus. In the DS9490B there is only space for
one iButton device. The ROM in the DS9490B is another 1-Wire device,
but it should ignore the DS1922 specific commmands.
- [ ] Handle rollover
- [ ] Support passwords
- [x] Handle low resolution mode
- [x] Add support for DS1922T
