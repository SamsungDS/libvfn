#!/bin/bash

set -euo pipefail

SOURCE_PARENT="0000:01:00.0"
SOURCE_CHILD="0000:01:00.1"
DESTINATION_PARENT="0000:02:00.0"
DESTINATION_CHILD="0000:02:00.1"

modprobe vfio-pci enable_sriov=1

for ctrl in "$SOURCE_PARENT" "$DESTINATION_PARENT"; do
	./build/tools/vfntool/vfntool -d "$ctrl" -v
done

# create vfs
echo 1 > "/sys/bus/pci/devices/${SOURCE_PARENT}/sriov_numvfs"
echo 1 > "/sys/bus/pci/devices/${DESTINATION_PARENT}/sriov_numvfs"

./build/examples/migrate \
	-p "$SOURCE_PARENT" -c "$SOURCE_CHILD" \
	-P "$DESTINATION_PARENT" -C "$DESTINATION_CHILD"
