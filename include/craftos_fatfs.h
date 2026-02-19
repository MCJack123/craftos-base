#ifndef CRAFTOS_FATFS_H
#define CRAFTOS_FATFS_H
#include <craftos.h>
#include <ff.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if FF_MIN_SS != FF_MAX_SS
#include <diskio.h>
#endif

#if FF_FS_MINIMIZE != 0
#error FatFs must not be minimized to use craftos_fatfs.
#endif

/** Includes all craftos_fatfs functions at once. */
#define CRAFTOS_FATFS_FUNCS \
    craftos_fatfs_fopen,\
    craftos_fatfs_fclose,\
    craftos_fatfs_fread,\
    craftos_fatfs_fwrite,\
    craftos_fatfs_fflush,\
    craftos_fatfs_fgetc,\
    craftos_fatfs_fputc,\
    craftos_fatfs_ftell,\
    craftos_fatfs_fseek,\
    craftos_fatfs_feof,\
    craftos_fatfs_ferror,\
    craftos_fatfs_remove,\
    craftos_fatfs_rename,\
    craftos_fatfs_mkdir,\
    craftos_fatfs_access,\
    craftos_fatfs_stat,\
    craftos_fatfs_statvfs,\
    craftos_fatfs_opendir,\
    craftos_fatfs_closedir,\
    craftos_fatfs_readdir

/** Includes all craftos_fatfs functions at once, using explicit field names. */
#define CRAFTOS_FATFS_FUNCS_BY_NAME \
    .fopen = craftos_fatfs_fopen,\
    .fclose = craftos_fatfs_fclose,\
    .fread = craftos_fatfs_fread,\
    .fwrite = craftos_fatfs_fwrite,\
    .fflush = craftos_fatfs_fflush,\
    .fgetc = craftos_fatfs_fgetc,\
    .fputc = craftos_fatfs_fputc,\
    .ftell = craftos_fatfs_ftell,\
    .fseek = craftos_fatfs_fseek,\
    .feof = craftos_fatfs_feof,\
    .ferror = craftos_fatfs_ferror,\
    .remove = craftos_fatfs_remove,\
    .rename = craftos_fatfs_rename,\
    .mkdir = craftos_fatfs_mkdir,\
    .access = craftos_fatfs_access,\
    .stat = craftos_fatfs_stat,\
    .statvfs = craftos_fatfs_statvfs,\
    .opendir = craftos_fatfs_opendir,\
    .closedir = craftos_fatfs_closedir,\
    .readdir = craftos_fatfs_readdir

static FILE * craftos_fatfs_fopen(const char * file, const char * mode, craftos_machine_t machine) {
    FIL* ptr = (FIL*)malloc(sizeof(FIL));
    FRESULT res = f_open(ptr, file, (mode[0] == 'r' ? FA_READ : FA_WRITE) | (mode[0] == 'w' ? FA_CREATE_ALWAYS : (mode[0] == 'a' ? FA_OPEN_APPEND : 0)) | (strchr(mode, '+') ? FA_READ | FA_WRITE : 0));
    if (res != FR_OK) {
        free(ptr);
        return NULL;
    }
    return (FILE*)ptr;
}

static int craftos_fatfs_fclose(FILE *fp, craftos_machine_t machine) {
    FRESULT res = f_close((FIL*)fp);
    free(fp);
    return res;
}

static size_t craftos_fatfs_fread(void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine) {
    UINT retval;
    FRESULT res = f_read((FIL*)fp, buf, size * count, &retval);
    if (res != FR_OK) return -1;
    return retval;
}

static size_t craftos_fatfs_fwrite(const void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine) {
    UINT retval;
    FRESULT res = f_write((FIL*)fp, buf, size * count, &retval);
    if (res != FR_OK) return -1;
    return retval;
}

static int craftos_fatfs_fflush(FILE * fp, craftos_machine_t machine) {
    return f_sync((FIL*)fp);
}

static int craftos_fatfs_fgetc(FILE * fp, craftos_machine_t machine) {
    char c;
    UINT read;
    if (f_read((FIL*)fp, &c, 1, &read) != FR_OK) return -1;
    if (read == 0) return EOF;
    return c;
}

static int craftos_fatfs_fputc(int ch, FILE * fp, craftos_machine_t machine) {
    return f_putc(ch, (FIL*)fp);
}

static long craftos_fatfs_ftell(FILE * fp, craftos_machine_t machine) {
    return f_tell((FIL*)fp);
}

static int craftos_fatfs_fseek(FILE * fp, long offset, int origin, craftos_machine_t machine) {
    switch (origin) {
        case SEEK_SET: return f_lseek((FIL*)fp, offset);
        case SEEK_CUR: return f_lseek((FIL*)fp, f_tell((FIL*)fp) + offset);
        case SEEK_END: return f_lseek((FIL*)fp, f_size((FIL*)fp) - offset);
        default: return -1;
    }
}

static int craftos_fatfs_feof(FILE * fp, craftos_machine_t machine) {
    return f_eof((FIL*)fp);
}

static int craftos_fatfs_ferror(FILE * fp, craftos_machine_t machine) {
    return f_error((FIL*)fp);
}

static int craftos_fatfs_remove(const char * path, craftos_machine_t machine) {
    return f_unlink(path);
}

static int craftos_fatfs_rename(const char * from, const char * to, craftos_machine_t machine) {
    return f_rename(from, to);
}

static int craftos_fatfs_mkdir(const char * path, int mode, craftos_machine_t machine) {
    return f_mkdir(path);
}

static int craftos_fatfs_access(const char * path, int flags, craftos_machine_t machine) {
    return 0;
}

static unsigned int fatfs_to_unix_time(unsigned short fdate, unsigned short ftime) {
    struct tm time_struct = {0};
    time_struct.tm_sec = (ftime & 0x1F) * 2;
    time_struct.tm_min = (ftime >> 5) & 0x3F;
    time_struct.tm_hour = (ftime >> 11) & 0x1F;
    time_struct.tm_mday = fdate & 0x1F;
    time_struct.tm_mon = ((fdate >> 5) & 0x0F) - 1;
    time_struct.tm_year = ((fdate >> 9) & 0x7F) + 80;
    return mktime(&time_struct);
}

static int craftos_fatfs_stat(const char * path, struct craftos_stat * st, craftos_machine_t machine) {
    FILINFO info;
    FRESULT res = f_stat(path, &info);
    if (res != 0) return res;
    st->st_dev = 0;
    st->st_ino = 0;
    st->st_mode = (info.fattrib & AM_DIR ? 0x4000 : 0) | (info.fattrib & AM_RDO ? 0777 : 0555);
    st->st_nlink = 0;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = info.fsize;
    st->st_blksize = 512;
    st->st_blocks = info.fsize / 512 + (info.fsize % 512 ? 1 : 0);
    st->st_atim.tv_sec = 0;
    st->st_atim.tv_nsec = 0;
    st->st_mtim.tv_sec = fatfs_to_unix_time(info.fdate, info.ftime);
    st->st_mtim.tv_nsec = 0;
#if FF_FS_CRTIME
    st->st_ctim.tv_sec = fatfs_to_unix_time(info.crdate, info.crtime);
#else
    st->st_ctim.tv_sec = 0;
#endif
    st->st_ctim.tv_nsec = 0;
    return 0;
}

static int craftos_fatfs_statvfs(const char * path, struct craftos_statvfs * st, craftos_machine_t machine) {
    FATFS* fs;
    DWORD free;
    FRESULT res = f_getfree(path, &free, &fs);
    if (res != FR_OK) return res;
#if FF_MIN_SS != FF_MAX_SS
    WORD SECTOR_SIZE;
    disk_ioctl(fs->pdrv, GET_SECTOR_SIZE, &SECTOR_SIZE);
#else
#define SECTOR_SIZE FF_MIN_SS
#endif
    st->f_bsize = fs->csize * SECTOR_SIZE;
    st->f_frsize = SECTOR_SIZE;
    st->f_blocks = fs->n_fatent - 2;
    st->f_bfree = free;
    st->f_bavail = free;
    st->f_files = fs->n_rootdir;
    st->f_ffree = 0;
    st->f_favail = 0;
    st->f_fsid = fs->id;
    st->f_flag = fs->fs_type;
#if FF_USE_LFN
    st->f_namemax = FF_MAX_LFN;
#else
    st->f_namemax = 12;
#endif
    return 0;
}

static craftos_DIR * craftos_fatfs_opendir(const char * path, craftos_machine_t machine) {
    DIR* d = (DIR*)malloc(sizeof(DIR));
    FRESULT res = f_opendir(d, path);
    if (res != FR_OK) {
        free(d);
        return NULL;
    }
    return (craftos_DIR*)d;
}

static int craftos_fatfs_closedir(craftos_DIR * dir, craftos_machine_t machine) {
    FRESULT res = f_closedir((DIR*)dir);
    free(dir);
    return res;
}

static struct craftos_dirent * craftos_fatfs_readdir(craftos_DIR * dir, craftos_machine_t machine) {
    static struct craftos_dirent d; /* not threadsafe! */
    FILINFO info;
    if (f_readdir((DIR*)dir, &info) != FR_OK || info.fname[0] == 0) return NULL;
    d.d_ino = 0;
    d.d_reclen = sizeof(craftos_dirent);
    d.d_type = info.fattrib;
    d.d_namlen = strlen(info.fname);
    strncpy(d.d_name, info.fname, 255);
    return &d;
}

#endif
