#include "../../include/linux/page.h"

struct page *mem_map;


void *high_memory;
unsigned long num_physpages;
unsigned long totalram_pages;
unsigned long max_mapnr;
unsigned long totalhigh_pages;
