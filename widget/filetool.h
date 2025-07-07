/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */


int filetool_copy_remote_to_local (const char *host, const char *remote_filename, const char *local_filename);
int filetool_copy_local_to_remote (const char *local_filename, const char *host, const char *remote_filename);
int filetool_process_args (int argc, char **argv);



