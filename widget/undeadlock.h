/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* undeadlock.h - undead terminal lock-file management
   Copyright (C) 1996-2022 Paul Sheer
 */

#ifndef UNDEADLOCK_H
#define UNDEADLOCK_H

struct cterminal_config;
struct undead_lock;

int undead_lock_fcntl (int fd, short l_type, int cmd);
struct undead_lock *undead_lock_alloc (const char *undead_path, struct cterminal_config *config);
struct undead_lock *undead_lock_find (const char *undead_path);
void undead_lock_free (struct undead_lock *udl);

#endif
