#!/usr/bin/env bash

dd if=/dev/random of=test.img bs=4k count=10
setfattr -n user.comment -v "$(echo test.img)" test.file
getfattr -n user.bigattr test.file
# CRC of the leaf block
./xfs_db -r -x /dev/sda5 -c 'inode 132' -c 'ablock 0' -c 'crc'
# CRC of the remote value block
./xfs_db -r -x /dev/sda5 -c 'inode 132' -c 'ablock 1' -c 'crc'
