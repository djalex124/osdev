exec-file drive/kernel.bin
symbol-file bin/kernel.map
target remote :1234

set print pretty on
b kscreen.c:239
c