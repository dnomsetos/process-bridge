#pragma once

#ifdef __x86_64__

#include <linux/x86_64/snapshot.h>

#elif defined(__i386__)

#include <linux/x86/snapshot.h>

#else

#error "Unsupported architecture"

#endif
