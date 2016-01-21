#include "linux/kernel.h"
#include "linux/fs.h"
#include <stdlib.h>

int root_mountflags = MS_RDONLY | MS_SILENT;
dev_t ROOT_DEV;
