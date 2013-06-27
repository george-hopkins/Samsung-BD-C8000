#!/bin/sh
# RFS symbolic link, SP Team 2009-08-31

RFS_DIR=rfs-1.3.1_b060

## create symbolic link files
pushd ./fs
ln -s ../$RFS_DIR/fs/rfs rfs
popd

pushd ./drivers
ln -s ../$RFS_DIR/drivers/fsr fsr
ln -s ../$RFS_DIR/drivers/tfsr tfsr
popd

pushd ./include/linux
ln -s ../../$RFS_DIR/include/linux/rfs_fs_sb.h rfs_fs_sb.h
ln -s ../../$RFS_DIR/include/linux/rfs_fs_i.h rfs_fs_i.h
ln -s ../../$RFS_DIR/include/linux/fsr_if.h fsr_if.h
ln -s ../../$RFS_DIR/include/linux/rfs_fs.h rfs_fs.h
popd
