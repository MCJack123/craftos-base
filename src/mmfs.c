/**
 * Memory-Mapped Filesystem for ESP32
 * A performance-oriented filesystem driver designed for read-only partitions.
 * 
 * Copyright (c) 2024 JackMacWindows. Licensed under the Apache 2.0 license.
 */

#include <errno.h>
#include <string.h>
#include <craftos.h>
#include "mmfs.h"

#define MMFS_MAGIC 0x73664D4D /* MMfs */
#define PATH_MAX 260

const struct mmfs_dir_ent* mmfs_traverse(const struct mmfs_dir* root, const char* pat) {
    /* Directory entries are sorted, so we use a binary sort on each level */
    static char path[PATH_MAX];
    const struct mmfs_dir_ent* node = NULL;
    const struct mmfs_dir* dir = root;
    char* p;
    unsigned int l, h, m;
    int res;
    strcpy(path, pat);
    for (p = strtok(path, "/"); p; p = strtok(NULL, "/")) {
        if (strcmp(p, "") == 0) continue;
        if (node) {
            if (!node->is_dir) {
                errno = ENOTDIR;
                return NULL;
            }
            dir = (const void*)root + node->offset;
        }
        if (dir->magic != MMFS_MAGIC) {
            errno = EIO;
            return NULL;
        }
        l = 0; h = dir->count;
        while (1) {
            if (l >= h) {
                errno = ENOENT;
                return NULL;
            }
            m = l + (h - l) / 2;
            res = strcmp(p, dir->entries[m].name);
            if (res == 0) {
                node = &dir->entries[m];
                break;
            } else if (res > 0) {
                l = m + 1;
            } else {
                h = m;
            }
        }
    }
    return node;
}
