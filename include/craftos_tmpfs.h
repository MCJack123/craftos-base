/**
 * craftos_tmpfs.h
 * craftos-base
 * 
 * Defines filesystem functions that can be used to bring up an in-memory
 * filesystem for use with craftos-base. This is useful for working with systems
 * without proper non-volatile storage.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#ifndef CRAFTOS_TMPFS_H
#define CRAFTOS_TMPFS_H
#include "craftos.h"

/** Includes all craftos_tmpfs functions at once. */
#define CRAFTOS_TMPFS_FUNCS \
    craftos_tmpfs_fopen,\
    craftos_tmpfs_fclose,\
    craftos_tmpfs_fread,\
    craftos_tmpfs_fwrite,\
    craftos_tmpfs_fflush,\
    craftos_tmpfs_fgetc,\
    craftos_tmpfs_fputc,\
    craftos_tmpfs_ftell,\
    craftos_tmpfs_fseek,\
    craftos_tmpfs_feof,\
    craftos_tmpfs_ferror,\
    craftos_tmpfs_remove,\
    craftos_tmpfs_rename,\
    craftos_tmpfs_mkdir,\
    craftos_tmpfs_access,\
    craftos_tmpfs_stat,\
    craftos_tmpfs_statvfs,\
    craftos_tmpfs_opendir,\
    craftos_tmpfs_closedir,\
    craftos_tmpfs_readdir

/** Includes all craftos_tmpfs functions at once, using explicit field names. */
#define CRAFTOS_TMPFS_FUNCS_BY_NAME \
    .fopen = craftos_tmpfs_fopen,\
    .fclose = craftos_tmpfs_fclose,\
    .fread = craftos_tmpfs_fread,\
    .fwrite = craftos_tmpfs_fwrite,\
    .fflush = craftos_tmpfs_fflush,\
    .fgetc = craftos_tmpfs_fgetc,\
    .fputc = craftos_tmpfs_fputc,\
    .ftell = craftos_tmpfs_ftell,\
    .fseek = craftos_tmpfs_fseek,\
    .feof = craftos_tmpfs_feof,\
    .ferror = craftos_tmpfs_ferror,\
    .remove = craftos_tmpfs_remove,\
    .rename = craftos_tmpfs_rename,\
    .mkdir = craftos_tmpfs_mkdir,\
    .access = craftos_tmpfs_access,\
    .stat = craftos_tmpfs_stat,\
    .statvfs = craftos_tmpfs_statvfs,\
    .opendir = craftos_tmpfs_opendir,\
    .closedir = craftos_tmpfs_closedir,\
    .readdir = craftos_tmpfs_readdir

/**
 * Initializes the tmpfs system, optionally filling from an existing mmfs.
 * @param init_mmfs A mmfs block to load files from, or NULL to create it empty
 * @return 0 on success, non-0 on error
 */
extern int craftos_tmpfs_init(const void * init_mmfs);

extern FILE * craftos_tmpfs_fopen(const char * file, const char * mode, craftos_machine_t machine);
extern int craftos_tmpfs_fclose(FILE *fp, craftos_machine_t machine);
extern size_t craftos_tmpfs_fread(void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine);
extern size_t craftos_tmpfs_fwrite(const void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_fflush(FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_fgetc(FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_fputc(int ch, FILE * fp, craftos_machine_t machine);
extern long craftos_tmpfs_ftell(FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_fseek(FILE * fp, long offset, int origin, craftos_machine_t machine);
extern int craftos_tmpfs_feof(FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_ferror(FILE * fp, craftos_machine_t machine);
extern int craftos_tmpfs_remove(const char * path, craftos_machine_t machine);
extern int craftos_tmpfs_rename(const char * from, const char * to, craftos_machine_t machine);
extern int craftos_tmpfs_mkdir(const char * path, int mode, craftos_machine_t machine);
extern int craftos_tmpfs_access(const char * path, int flags, craftos_machine_t machine);
extern int craftos_tmpfs_stat(const char * path, struct craftos_stat * st, craftos_machine_t machine);
extern int craftos_tmpfs_statvfs(const char * path, struct craftos_statvfs * st, craftos_machine_t machine);
extern craftos_DIR * craftos_tmpfs_opendir(const char * path, craftos_machine_t machine);
extern int craftos_tmpfs_closedir(craftos_DIR * dir, craftos_machine_t machine);
extern struct craftos_dirent * craftos_tmpfs_readdir(craftos_DIR * dir, craftos_machine_t machine);

#endif