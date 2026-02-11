/**
 * Memory-Mapped Filesystem for ESP32
 * A performance-oriented filesystem driver designed for read-only partitions.
 * 
 * Copyright (c) 2024 JackMacWindows. Licensed under the Apache 2.0 license.
 */

#ifndef MMFS_H
#define MMFS_H

#ifndef __packed
#ifdef __GNUC__
#define __packed __attribute__((packed))
#else
#define __packed
#endif
#endif

struct mmfs_dir_ent {
    const char name[24];
    const unsigned is_dir: 1;
    const unsigned size: 31;
    const unsigned int offset;
} __packed;

struct mmfs_dir {
    const unsigned int magic;
    const unsigned int count;
    const struct mmfs_dir_ent entries[];
} __packed;

struct mmfs_fd {
    const unsigned char* start;
    const unsigned char* ptr;
    const unsigned char* end;
};

extern const struct mmfs_dir_ent * mmfs_traverse(const struct mmfs_dir * root, const char * path);
#define mmfs_get_data(root, file) ((const unsigned char *)(root) + (file)->offset)

#endif
