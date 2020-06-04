#!/bin/sh
dd if=/dev/zero bs=2352 count=300 of=audio.raw
cdrecord dev=/dev/cdrom -multi -audio audio.raw
MSINFO=$(cdrecord dev=/dev/cdrom -msinfo | tail -n 1)
mkisofs -l -d -relaxed-filenames -C "${MSINFO}" -o tmp.iso cd
( cat IP.BIN ; dd if=tmp.iso bs=2048 skip=16 ) > data.raw
cdrecord dev=/dev/cdrom -multi -xa data.raw
rm -f audio.raw data.raw tmp.iso
