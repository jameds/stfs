/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <errno.h>
#include <fuse.h>
#include "int.h"

int fs_getattr();
int fs_mkdir();
int fs_readdir();
int fs_readlink();
int fs_rename();
int fs_rmdir();
int fs_unlink();
