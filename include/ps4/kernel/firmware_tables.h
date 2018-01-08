#pragma once

typedef struct sym_t
{
	const char *name;
	int offset;
} sym_t;

sym_t table405[] = {
	#include "./firmware_tables/405_kernel_sym.inc"
	{ NULL, 0 },
};
