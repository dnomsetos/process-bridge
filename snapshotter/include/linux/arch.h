#pragma once

#include <stddef.h>

#define ASSERT_SAME_FIELD(a, b, field)                               \
  _Static_assert(offsetof(a, field) == offsetof(b, field),           \
                 "field offset mismatch: " #field);                  \
  _Static_assert(sizeof(((a *)0)->field) == sizeof(((b *)0)->field), \
                 "field size mismatch: " #field)

#ifdef __x86_64__

#include <linux/x86_64/process_state.h>
typedef linux_x86_64_process_state_t process_state_t;

_Static_assert(sizeof(struct pb_x86_64_regs) == sizeof(struct user_regs_struct),
               "struct size mismatch");

ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r15);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r14);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r13);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r12);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rbp);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rbx);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r11);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r10);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r9);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, r8);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rax);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rcx);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rdx);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rsi);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rdi);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, orig_rax);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rip);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, cs);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, eflags);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, rsp);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, ss);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, fs_base);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, gs_base);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, ds);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, es);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, fs);
ASSERT_SAME_FIELD(struct pb_x86_64_regs, struct user_regs_struct, gs);

#elif defined(__i386__)

#include <linux/i386/process_state.h>
typedef linux_i386_process_state_t process_state_t;

_Static_assert(sizeof(struct pb_i386_regs) == sizeof(struct user_regs_struct),
               "struct size mismatch");

ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, ebx);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, ecx);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, edx);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, esi);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, edi);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, ebp);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, eax);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xds);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xes);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xfs);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xgs);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, orig_eax);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, eip);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xcs);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, eflags);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, esp);
ASSERT_SAME_FIELD(struct pb_i386_regs, struct user_regs_struct, xss);

#else

#error "Unsupported architecture"

#endif
