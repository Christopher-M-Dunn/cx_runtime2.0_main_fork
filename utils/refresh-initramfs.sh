mkdir initramfs
pushd initramfs
mkdir -p {bin,sbin,dev,etc,home,mnt,proc,sys,usr,tmp}
mkdir -p usr/{bin,sbin}
mkdir -p proc/sys/kernel
pushd dev
sudo mknod sda b 8 0 
sudo mknod console c 5 1
popd

rm -f initramfs.cpio.gz

chmod +x init

find . -print0 | cpio --null -ov --format=newc | gzip -9 > initramfs.cpio.gz

popd
