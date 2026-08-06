#pragma once

#ifdef __linux__

#include <linux/arch.h>
#include <linux/breakpoint.h>
#include <linux/mappings_reader.h>
#include <linux/process_info.h>
#include <linux/snapshot_dump.h>

#else

#error "Platform not supported"

#endif
