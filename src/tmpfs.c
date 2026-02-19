#include <craftos.h>
#include <craftos_tmpfs.h>
#include "types.h"
#include <stdlib.h>
#include <string.h>

int craftos_tmpfs_init(const void * init_mmfs) {

}

FILE * craftos_tmpfs_fopen(const char * file, const char * mode, craftos_machine_t machine) {

}

int craftos_tmpfs_fclose(FILE *fp, craftos_machine_t machine) {
    
}

size_t craftos_tmpfs_fread(void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine) {

}

size_t craftos_tmpfs_fwrite(const void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_fflush(FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_fgetc(FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_fputc(int ch, FILE * fp, craftos_machine_t machine) {

}

long craftos_tmpfs_ftell(FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_fseek(FILE * fp, long offset, int origin, craftos_machine_t machine) {

}

int craftos_tmpfs_feof(FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_ferror(FILE * fp, craftos_machine_t machine) {

}

int craftos_tmpfs_remove(const char * path, craftos_machine_t machine) {

}

int craftos_tmpfs_rename(const char * from, const char * to, craftos_machine_t machine) {

}

int craftos_tmpfs_mkdir(const char * path, int mode, craftos_machine_t machine) {

}

int craftos_tmpfs_access(const char * path, int flags, craftos_machine_t machine) {

}

int craftos_tmpfs_stat(const char * path, struct craftos_stat * st, craftos_machine_t machine) {

}

int craftos_tmpfs_statvfs(const char * path, struct craftos_statvfs * st, craftos_machine_t machine) {

}

craftos_DIR * craftos_tmpfs_opendir(const char * path, craftos_machine_t machine) {

}

int craftos_tmpfs_closedir(craftos_DIR * dir, craftos_machine_t machine) {

}

struct craftos_dirent * craftos_tmpfs_readdir(craftos_DIR * dir, craftos_machine_t machine) {

}
