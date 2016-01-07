#include "../../include/linux/kernel.h"
#include "../../include/linux/fs.h"

int root_mountflags = MS_RDONLY | MS_SILENT;
dev_t ROOT_DEV;
