WinCE 7 (Windows Embedded Compact 7) BSP for QEMU vexpress-a15

expected configuration:
- 256MB RAM
- PL011 for kernel debug output (UART0) and KITL (UART2)
- PL111 display (1024x600)
- PL050 keyboard/mouse
- PL011 for COM1 port (UART1)
- PL041 audio output
- virtio-tablet-device (on 0x91130600)
- virtio-blk-device (on 0x91130400)

you will need to convert generated NK.bin to flat image, then load it at 0x80000000 and set PC to entry point

example usage (image available on github releases):
```
qemu-system-arm \
-global virtio-mmio.force-legacy=false \
-machine type=vexpress-a15 -m 256 \
-device loader,addr=0x80000000,file=NK.bin.entry-0x800010c0.raw \
-device loader,addr=0x800010c0,cpu-num=0 \
-device virtio-tablet-device \
-blockdev driver=file,filename=disk.img,node-name=d0 -device virtio-blk-device,drive=d0
```
