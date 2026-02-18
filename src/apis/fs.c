#include <craftos.h>
#include "../types.h"
#include "../mmfs.h"
#include "../string_list.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#ifndef __noreturn
#ifdef __GNUC__
#define __noreturn __attribute__((noreturn))
#else
#define __noreturn
#endif
#endif

extern int fs_handle_close(lua_State *L);
extern int fs_handle_gc(lua_State *L);
extern int fs_handle_readAll(lua_State *L);
extern int fs_handle_readLine(lua_State *L);
extern int fs_handle_readByte(lua_State *L);
extern int fs_handle_readAllByte(lua_State *L);
extern int fs_handle_writeString(lua_State *L);
extern int fs_handle_writeLine(lua_State *L);
extern int fs_handle_writeByte(lua_State *L);
extern int fs_handle_flush(lua_State *L);
extern int fs_handle_seek(lua_State *L);

extern int fs_handle_mmfs_close(lua_State *L);
extern int fs_handle_mmfs_gc(lua_State *L);
extern int fs_handle_mmfs_readAll(lua_State *L);
extern int fs_handle_mmfs_readLine(lua_State *L);
extern int fs_handle_mmfs_readByte(lua_State *L);
extern int fs_handle_mmfs_readAllByte(lua_State *L);
extern int fs_handle_mmfs_seek(lua_State *L);

#ifdef STANDALONE_ROM
extern FileEntry standaloneROM;
extern FileEntry standaloneDebug;
extern std::string standaloneBIOS;
#endif

static const char * ignored_files[4] = {
    ".",
    "..",
    ".DS_Store",
    "desktop.ini"
};

static int strall(const char * str, char c) {
    while (*str) if (*str++ != c) return 0;
    return 1;
}

static char* assemblePath(const char * base, const struct string_list * path) {
    struct string_list_node * n;
    char* retval;
    size_t len = strlen(base) + 1;
    if (len == 1) len--; /* will be compensated in next loop */
    for (n = path->head; n; n = n->next) len += 1 + strlen(n->str);
    if (len == 0) len = 1; /* or not maybe */
    retval = F.malloc(len);
    *retval = 0;
    if (*base) strcpy(retval, base);
    for (n = path->head; n; n = n->next) {
        if (*retval) strcat(retval, "/");
        strcat(retval, n->str);
    }
    return retval;
}

static char* fixpath(craftos_machine_t comp, const char * path, int exists, int addExt, const struct craftos_mount_list ** mount) {
    char* ss;
    struct string_list pathc = {NULL, NULL};
    if (string_split_path(path, &pathc, addExt) != 0) return NULL;
    while (pathc.head != NULL && *pathc.head->str == 0) string_list_shift(&pathc);
    if (pathc.tail != NULL && strlen(pathc.tail->str) > 255)
        pathc.tail->str[256] = 0;
    if (addExt) {
        size_t max_path_n = 0, max_path_l_n = 1;
        const struct craftos_mount_list * max_path_l[8] = {comp->mounts, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        const struct craftos_mount_list * m;
        for (m = comp->mounts; m != NULL; m = m->next) {
            char** pathlist = m->mount_path;
            int i;
            struct string_list_node * pathc_iter;
            for (pathc_iter = pathc.head, i = 0; pathc_iter != NULL; pathc_iter = pathc_iter->next, i++) {
                if (pathlist[i] == NULL) {
                    if (i > max_path_n) {
                        max_path_n = i;
                        memset(max_path_l, 0, sizeof(max_path_l));
                        max_path_l[0] = m;
                        max_path_l_n = 1;
                    } else if (i == max_path_n && max_path_l_n < 8) {
                        max_path_l[max_path_l_n++] = m;
                    }
                    i = -1;
                    break;
                }
            }
            if (i >= 0 && pathlist[i] == NULL) {
                if (i > max_path_n) {
                    max_path_n = i;
                    memset(max_path_l, 0, sizeof(max_path_l));
                    max_path_l[0] = m;
                    max_path_l_n = 1;
                } else if (i == max_path_n && max_path_l_n < 8) {
                    max_path_l[max_path_l_n++] = m;
                }
            }
        }
        while (max_path_n--) string_list_shift(&pathc);
        if (exists) {
            int found = 0, i;
            for (i = 0; i < max_path_l_n; i++) {
                if (max_path_l[i]->flags & MOUNT_FLAG_MMFS) {
                    char* fullpath = assemblePath("", &pathc);
                    if (mmfs_traverse(max_path_l[i]->root_dir, fullpath) != NULL) {
                        ss = fullpath;
                        if (mount) *mount = max_path_l[i];
                        found = 1;
                        break;
                    } else F.free(fullpath);
                } else {
                    char* fullpath = assemblePath(max_path_l[i]->filesystem_path, &pathc);
                    struct craftos_stat st;
                    if (F.stat(fullpath, &st, comp) == 0) {
                        ss = fullpath;
                        if (mount) *mount = max_path_l[i];
                        found = 1;
                        break;
                    } else F.free(fullpath);
                }
            }
            if (!found) {
                string_list_clear(&pathc);
                return NULL;
            }
        } else if (pathc.head && pathc.head->next && pathc.head->next->next) {
            int found = 0, i;
            struct string_list oldback;
            while (!found && pathc.head) {
                for (i = 0; i < max_path_l_n; i++) {
                    if (max_path_l[i]->flags & MOUNT_FLAG_MMFS) {
                        char* fullpath = assemblePath("", &pathc);
                        if (mmfs_traverse(max_path_l[i]->root_dir, fullpath) != NULL) {
                            ss = assemblePath(fullpath, &oldback);
                            F.free(fullpath);
                            if (mount) *mount = max_path_l[i];
                            found = 1;
                            break;
                        } else {
                            char* p = strrchr(fullpath, '/');
                            if (p != NULL) {
                                const struct mmfs_dir_ent * ent;
                                *p = 0;
                                if ((ent = mmfs_traverse(max_path_l[i]->root_dir, fullpath)) != NULL && ent->is_dir) {
                                    *p = '/';
                                    ss = assemblePath(fullpath, &oldback);
                                    F.free(fullpath);
                                    if (mount) *mount = max_path_l[i];
                                    found = 1;
                                    break;
                                }
                            }
                        }
                        F.free(fullpath);
                    } else {
                        char* fullpath = assemblePath(max_path_l[i]->filesystem_path, &pathc);
                        struct craftos_stat st;
                        if (F.stat(fullpath, &st, comp) == 0) {
                            ss = assemblePath(fullpath, &oldback);
                            F.free(fullpath);
                            if (mount) *mount = max_path_l[i];
                            found = 1;
                            break;
                        } else {
                            char* p = strrchr(fullpath, '/');
                            if (p != NULL) {
                                *p = 0;
                                if (F.stat(fullpath, &st, comp) == 0 && S_ISDIR(st.st_mode)) {
                                    *p = '/';
                                    ss = assemblePath(fullpath, &oldback);
                                    F.free(fullpath);
                                    if (mount) *mount = max_path_l[i];
                                    found = 1;
                                    break;
                                }
                            }
                        }
                        F.free(fullpath);
                    }
                }
                if (!found) string_list_push_pop(&oldback, &pathc);
            }
            string_list_clear(&oldback);
            if (!found) {
                string_list_clear(&pathc);
                return NULL;
            }
        } else {
            ss = assemblePath(max_path_l[0]->flags & MOUNT_FLAG_MMFS ? "" : max_path_l[0]->filesystem_path, &pathc);
            if (mount) *mount = max_path_l[0];
        }
    } else ss = assemblePath("", &pathc);
    string_list_clear(&pathc);
    return ss;
}

static int fixpath_ro(craftos_machine_t comp, const char * path) {
    const struct craftos_mount_list * mount;
    char* p = fixpath(comp, path, 1, 1, &mount);
    F.free(p);
    return mount->flags & MOUNT_FLAG_RO;
}

static char* fixpath_mkdir(craftos_machine_t comp, const char * path, int md, const struct craftos_mount_list ** mount) {
    char * s, * s2;
    char* ss, *firstTest, *pathfix;
    const char * pathptr = path;
    struct string_list pathc = {NULL, NULL}, append = {NULL, NULL};
    if (md && fixpath_ro(comp, path)) return NULL;
    firstTest = fixpath(comp, path, 1, 1, mount);
    if (firstTest != NULL) return firstTest;
    if (string_split_path(path, &pathc, 1) != 0) return NULL;
    if (pathc.head == NULL) return fixpath(comp, "", 1, 1, mount);
    string_list_pop(&pathc);
    pathfix = assemblePath("", &pathc);
    char* maxPath = fixpath(comp, pathfix, 0, 1, mount);
    F.free(pathfix);
    while (maxPath == NULL) {
        string_list_push_pop(&append, &pathc);
        if (pathc.head == NULL) {
            string_list_clear(&append);
            return NULL;
        }
        pathfix = assemblePath("", &pathc);
        maxPath = fixpath(comp, pathfix, 0, 1, mount);
        F.free(pathfix);
    }
    if (!md) {
        string_list_clear(&append);
        string_list_clear(&pathc);
        return maxPath;
    }
    while (append.head != NULL) {
        int res = F.mkdir(maxPath, 0777, comp);
        F.free(maxPath);
        if (res != 0) {
            string_list_clear(&append);
            string_list_clear(&pathc);
            return NULL;
        }
        string_list_push_pop(&pathc, &append);
        pathfix = assemblePath("", &pathc);
        maxPath = fixpath(comp, pathfix, 0, 1, mount);
        F.free(pathfix);
    }
    string_list_clear(&pathc);
    return maxPath;
}

struct fixpath_multiple {
    int n;
    char * paths[8];
    const struct craftos_mount_list * mounts[8];
};

static int fixpath_multiple(craftos_machine_t comp, const char * path, struct fixpath_multiple * retval) {
    char * s, * s2;
    char * pathfix = F.malloc(strlen(path) + 1);
    struct string_list pathc = {NULL, NULL};
    if (string_split_path(path, &pathc, 1) != 0) return -1;
    while (pathc.head != NULL && *pathc.head->str == 0) string_list_shift(&pathc);
    if (pathc.tail != NULL && strlen(pathc.tail->str) > 255)
        pathc.tail->str[256] = 0;
    size_t max_path_n = 0;
    const struct craftos_mount_list * m;
    memset(retval, 0, sizeof(struct fixpath_multiple));
    retval->mounts[0] = comp->mounts;
    retval->n = 1;
    for (m = comp->mounts; m != NULL; m = m->next) {
        char** pathlist = m->mount_path;
        int i;
        struct string_list_node * pathc_iter;
        for (pathc_iter = pathc.head, i = 0; pathc_iter != NULL; pathc_iter = pathc_iter->next, i++) {
            if (pathlist[i] == NULL) {
                if (i > max_path_n) {
                    max_path_n = i;
                    memset(retval->mounts, 0, sizeof(retval->mounts));
                    retval->mounts[0] = m;
                    retval->n = 1;
                } else if (i == max_path_n && retval->n < 8) {
                    retval->mounts[retval->n++] = m;
                }
                i = -1;
                break;
            }
        }
        if (i >= 0 && pathlist[i] == NULL) {
            if (i > max_path_n) {
                max_path_n = i;
                memset(retval->mounts, 0, sizeof(retval->mounts));
                retval->mounts[0] = m;
                retval->n = 1;
            } else if (i == max_path_n && retval->n < 8) {
                retval->mounts[retval->n++] = m;
            }
        }
    }
    while (max_path_n--) string_list_shift(&pathc);
    int i, ok = -1;
    for (i = 0; i < retval->n; i++) {
        if (retval->mounts[i]->flags & MOUNT_FLAG_MMFS) {
            char* fullpath = assemblePath("", &pathc);
            if (mmfs_traverse(retval->mounts[i]->root_dir, fullpath) != NULL) {
                retval->paths[i] = fullpath;
                ok = 0;
            } else {
                retval->mounts[i] = NULL;
                F.free(fullpath);
            }
        } else {
            char* fullpath = assemblePath(retval->mounts[i]->filesystem_path, &pathc);
            struct craftos_stat st;
            if (F.stat(fullpath, &st, comp) == 0) {
                retval->paths[i] = fullpath;
                ok = 0;
            } else {
                retval->mounts[i] = NULL;
                F.free(fullpath);
            }
        }
    }
    string_list_clear(&pathc);
    return ok;
}

static char* normalizePath(const char * basePath, int allowWildcards) {
    char* s, *s2, *s3;
    char* cleanPath = F.malloc(1);
    char* pathfix = F.malloc(strlen(basePath) + 1);
    *cleanPath = 0;
    strcpy(pathfix, basePath);
    for (s = strtok(pathfix, "/\\"); (s = strtok(NULL, "/\\"));) {
        s2 = F.malloc(strlen(s) + 1);
        for (s3 = s2; *s; s++) {
            char c = *s;
            if (!(c == '"' || c == '*' || c == ':' || c == '<' || c == '>' || c == '?' || c == '|' || c < 32)) *s3++ = c;
        }
        /* TODO: space normalization */
        if (strcmp(s2, "..") == 0) {
            char* pos = strrchr(cleanPath, '/');
            if (pos != NULL) *pos = 0;
            else if (*cleanPath == 0) {
                F.free(s2);
                F.free(cleanPath);
                return NULL;
            } else *cleanPath = 0;
        } else if (*s2 != 0 && !(strall(s2, '.') && strlen(s2))) {
            cleanPath = F.realloc(cleanPath, strlen(cleanPath) + strlen(s2) + 2);
            if (*cleanPath != 0) strcat(cleanPath, "/");
            strcat(cleanPath, s2);
        }
        F.free(s2);
    }
    return cleanPath;
}

static void __noreturn err(lua_State *L, int idx, const char * err) {
    char * path = fixpath(get_comp(L), lua_tostring(L, idx), 0, 0, NULL);
    luaL_where(L, 1);
    lua_pushfstring(L, "/%s: %s", path, err);
    F.free(path);
    lua_concat(L, 2);
    lua_error(L);
}

static int fs_list(lua_State *L) {
    struct fixpath_multiple m;
    const char * str = luaL_checkstring(L, 1);
    craftos_machine_t machine = get_comp(L);
    if (fixpath_multiple(machine, str, &m) != 0) err(L, 1, "No such file");
    int gotdir = 0, i, j, n = 1;
    lua_newtable(L);
    for (i = 0; i < m.n; i++) {
        if (m.paths[i] != NULL) {
            if (m.mounts[i]->flags & MOUNT_FLAG_MMFS) {
                const struct mmfs_dir_ent * ent = mmfs_traverse(m.mounts[i]->root_dir, m.paths[i]);
                if (ent != NULL && ent->is_dir) {
                    const struct mmfs_dir * dir = mmfs_get_dir(m.mounts[i]->root_dir, ent);
                    gotdir = 1;
                    for (j = 0; j < dir->count; j++) {
                        lua_pushstring(L, dir->entries[j].name);
                        lua_rawseti(L, -2, n++);
                    }
                }
            } else {
                craftos_DIR* d = F.opendir(m.paths[i], machine);
                if (d != NULL) {
                    struct craftos_dirent * dir;
                    gotdir = 1;
                    while ((dir = F.readdir(d, machine))) {
                        lua_pushlstring(L, dir->d_name, dir->d_namlen);
                        lua_rawseti(L, -2, n++);
                    }
                    F.closedir(d, machine);
                }
            }
            F.free(m.paths[i]);
        }
    }
    if (!gotdir) err(L, 1, "Not a directory");
    struct string_list pathc = {NULL, NULL};
    string_split_path(str, &pathc, 1);
    for (i = 0; pathc.head; string_list_shift(&pathc), i++);
    const struct craftos_mount_list * mount;
    for (mount = machine->mounts; mount; mount = mount->next) {
        gotdir = 1;
        for (j = 0; j < i; j++) if (mount->mount_path[j] == NULL) {gotdir = 0; break;}
        if (gotdir && mount->mount_path[i] != NULL) {
            lua_pushstring(L, mount->mount_path[i]);
            lua_rawseti(L, -2, n++);
        }
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "tsort");
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 1;
}

static int fs_exists(lua_State *L) {
    const struct craftos_mount_list * mount;
    char* path = fixpath(get_comp(L), luaL_checkstring(L, 1), 1, 1, &mount);
    if (path != NULL && mount->flags & MOUNT_FLAG_MMFS) {
        lua_pushboolean(L, mmfs_traverse(mount->root_dir, path) != NULL);
    } else {
        lua_pushboolean(L, path != NULL);
    }
    F.free(path);
    return 1;
}

static int fs_isDir(lua_State *L) {
    const struct craftos_mount_list * mount;
    char* path = fixpath(get_comp(L), luaL_checkstring(L, 1), 1, 1, &mount);
    if (path != NULL && mount->flags & MOUNT_FLAG_MMFS) {
        const struct mmfs_dir_ent * ent;
        lua_pushboolean(L, (ent = mmfs_traverse(mount->root_dir, path)) != NULL && ent->is_dir);
    } else {
        struct craftos_stat st;
        lua_pushboolean(L, path != NULL && F.stat(path, &st, get_comp(L)) == 0 && S_ISDIR(st.st_mode));
    }
    F.free(path);
    return 1;
}

static int fs_isReadOnly(lua_State *L) {
    const struct craftos_mount_list * mount;
    struct craftos_stat st;
    char* path;
    const char * str = luaL_checkstring(L, 1);
    if (fixpath_ro(get_comp(L), str)) {
        lua_pushboolean(L, 1);
        return 1;
    }
    path = fixpath_mkdir(get_comp(L), str, 0, &mount);
    if (path == NULL) err(L, 1, "Invalid path"); /* This should never happen */
    if (F.stat(path, &st, get_comp(L)) != 0) lua_pushboolean(L, 0);
    else lua_pushboolean(L, F.access(path, W_OK, get_comp(L)) != 0);
    F.free(path);
    return 1;
}

static int fs_getName(lua_State *L) {
    char* path = normalizePath(luaL_checkstring(L, 1), 1);
    char* retval = strrchr(path, '/');
    if (retval == NULL) lua_pushliteral(L, "root");
    else lua_pushstring(L, retval + 1);
    F.free(path);
    return 1;
}

static int fs_getDrive(lua_State *L) {
    const struct craftos_mount_list * mount;
    char* path = fixpath(get_comp(L), luaL_checkstring(L, 1), 1, 1, &mount);
    F.free(path);
    if (mount->mount_path[0] == NULL) lua_pushliteral(L, "hdd");
    else {
        luaL_Buffer b;
        int i;
        luaL_buffinit(L, &b);
        luaL_addstring(&b, mount->mount_path[0]);
        for (i = 1; mount->mount_path[i]; i++) {
            luaL_addchar(&b, '/');
            luaL_addstring(&b, mount->mount_path[i]);
        }
        luaL_pushresult(&b);
    }
    return 1;
}

static int fs_getSize(lua_State *L) {
    const struct craftos_mount_list * mount;
    char* path = fixpath(get_comp(L), luaL_checkstring(L, 1), 1, 1, &mount);
    if (path == NULL) err(L, 1, "No such file");
    if (mount->flags & MOUNT_FLAG_MMFS) {
        const struct mmfs_dir_ent * f = mmfs_traverse(mount->root_dir, path);
        if (f == NULL) {
            F.free(path);
            err(L, 1, "No such file");
        } else if (f->is_dir) lua_pushinteger(L, 0);
        else lua_pushinteger(L, f->size);
    } else {
        struct craftos_stat st;
        if (F.stat(path, &st, get_comp(L)) != 0) {
            F.free(path);
            err(L, 1, "No such file");
        } else if (S_ISDIR(st.st_mode)) lua_pushinteger(L, 0);
        else lua_pushinteger(L, st.st_size);
    }
    F.free(path);
    return 1;
}

static int fs_getFreeSpace(lua_State *L) {
    const struct craftos_mount_list * mount;
    const char * str = luaL_checkstring(L, 1);
    char* path = fixpath(get_comp(L), str, 0, 1, &mount);
    if (path == NULL) err(L, 1, "No such path");
    if (mount->flags & MOUNT_FLAG_RO) lua_pushinteger(L, 0);
    else {
        struct craftos_statvfs st;
        if (F.statvfs(path, &st, get_comp(L)) != 0) {
            F.free(path);
            err(L, 1, "No such path");
        }
        lua_pushinteger(L, st.f_bavail * st.f_bsize);
    }
    F.free(path);
    return 1;
}

static int fs_makeDir(lua_State *L) {
    const struct craftos_mount_list * mount;
    const char * str = luaL_checkstring(L, 1);
    if (fixpath_ro(get_comp(L), str)) err(L, 1, "Access denied");
    char* path = fixpath_mkdir(get_comp(L), str, 1, &mount);
    if (path == NULL) err(L, 1, "Could not create directory");
    if (mount->flags & MOUNT_FLAG_RO) {
        F.free(path);
        err(L, 1, "Permission denied");
    }
    if (F.mkdir(path, 0777, get_comp(L)) != 0) {
        F.free(path);
        err(L, 1, strerror(errno));
    }
    F.free(path);
    return 0;
}

static int fs_move(lua_State *L) {
    const struct craftos_mount_list * mount1, * mount2;
    struct craftos_stat st;
    const char * str1 = luaL_checkstring(L, 1);
    const char * str2 = luaL_checkstring(L, 2);
    char* fromPath = fixpath(get_comp(L), str1, 1, 1, &mount1);
    char* toPath = fixpath_mkdir(get_comp(L), str2, 1, &mount2);
    if (fromPath == NULL) {
        if (toPath) F.free(toPath);
        luaL_error(L, "No such file");
    }
    if (toPath == NULL) {
        F.free(fromPath);
        err(L, 2, "Invalid path");
    }
    if (mount1->flags & MOUNT_FLAG_RO) {
        F.free(fromPath);
        F.free(toPath);
        err(L, 1, "Permission denied");
    }
    if (mount2->flags & MOUNT_FLAG_RO) {
        F.free(fromPath);
        F.free(toPath);
        err(L, 2, "Permission denied");
    }
    if (strlen(toPath) > strlen(fromPath) && strncmp(toPath, fromPath, strlen(fromPath)) == 0 && toPath[strlen(fromPath)] == '/') { 
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "Can't move a directory inside itself");
    }
    if (*fromPath == 0 || *toPath == 0 || (fromPath[0] == '/' && fromPath[1] == 0) || (toPath[0] == '/' && toPath[1] == 0)) {
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "Cannot move mount");
    }
    if (F.stat(toPath, &st, get_comp(L)) == 0) {
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "File exists");
    }
    if (F.rename(fromPath, toPath, get_comp(L)) != 0) {
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, strerror(errno));
    }
    F.free(fromPath);
    F.free(toPath);
    return 0;
}

static int copy_mmfs(const struct mmfs_dir * root, const struct mmfs_dir_ent * src, const char * dest, craftos_machine_t machine) {
    if (src->is_dir) {
        int res, i;
        char* newpath, *newpath_start;
        const struct mmfs_dir * dir = mmfs_get_dir(root, src);
        if ((res = F.mkdir(dest, 0777, machine)) != 0) return res;
        newpath = F.malloc(strlen(dest) + 26); 
        strcpy(newpath, dest);
        strcat(newpath, "/");
        newpath_start = newpath + strlen(dest) + 1;
        for (i = 0; i < dir->count; i++) {
            strcpy(newpath, dir->entries[i].name);
            if ((res = copy_mmfs(root, &dir->entries[i], newpath, machine)) != 0) {
                F.free(newpath);
                return res;
            }
        }
        F.free(newpath);
    } else {
        FILE* fp = F.fopen(dest, "wb", machine);
        if (fp == NULL) return -1;
        F.fwrite(mmfs_get_data(root, src), 1, src->size, fp, machine);
        F.fclose(fp, machine);
    }
    return 0;
}

static int copy_fs(const char * src, const char * dest, craftos_machine_t machine) {
    struct craftos_stat st;
    int res;
    if ((res = F.stat(src, &st, machine)) != 0) return res;
    if (S_ISDIR(st.st_mode)) {
        int res;
        char* newsrc, *newdest;
        craftos_DIR* dir;
        struct craftos_dirent * ent;
        if ((res = F.mkdir(dest, 0777, machine)) != 0) return res;
        dir = F.opendir(src, machine);
        if (dir == NULL) return -1;
        while ((ent = F.readdir(dir, machine)) != NULL) {
            newsrc = F.malloc(strlen(src) + ent->d_namlen + 2); 
            strcpy(newsrc, src);
            strcat(newsrc, "/");
            strcat(newsrc, ent->d_name);
            newdest = F.malloc(strlen(dest) + ent->d_namlen + 2); 
            strcpy(newdest, dest);
            strcat(newdest, "/");
            strcat(newdest, ent->d_name);
            if ((res = copy_fs(newsrc, newdest, machine)) != 0) {
                F.free(newsrc);
                F.free(newdest);
                return res;
            }
            F.free(newsrc);
            F.free(newdest);
        }
        F.closedir(dir, machine);
    } else {
        char buf[512];
        FILE* fpin = F.fopen(src, "rb", machine), *fpout;
        if (fpin == NULL) return -1;
        fpout = F.fopen(dest, "wb", machine);
        if (fpout == NULL) {
            F.fclose(fpin, machine);
            return -1;
        }
        while (!F.feof(fpin, machine)) {
            size_t sz = F.fread(buf, 1, 512, fpin, machine);
            F.fwrite(buf, 1, sz, fpout, machine);
            if (sz < 512) break;
        }
        F.fclose(fpin, machine);
        F.fclose(fpout, machine);
    }
    return 0;
}

static int fs_copy(lua_State *L) {
    const struct craftos_mount_list * mount1, * mount2;
    struct craftos_stat st;
    int res;
    const char * str1 = luaL_checkstring(L, 1);
    const char * str2 = luaL_checkstring(L, 2);
    char* fromPath = fixpath(get_comp(L), str1, 1, 1, &mount1);
    char* toPath = fixpath_mkdir(get_comp(L), str2, 1, &mount2);
    if (fromPath == NULL) {
        if (toPath) F.free(toPath);
        luaL_error(L, "No such file");
    }
    if (toPath == NULL) {
        F.free(fromPath);
        err(L, 2, "Invalid path");
    }
    if (mount2->flags & MOUNT_FLAG_RO) {
        F.free(fromPath);
        F.free(toPath);
        err(L, 2, "Permission denied");
    }
    if (strlen(toPath) > strlen(fromPath) && strncmp(toPath, fromPath, strlen(fromPath)) == 0 && toPath[strlen(fromPath)] == '/') { 
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "Can't copy a directory inside itself");
    }
    if (*fromPath == 0 || *toPath == 0 || (fromPath[0] == '/' && fromPath[1] == 0) || (toPath[0] == '/' && toPath[1] == 0)) {
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "Cannot move mount");
    }
    if (F.stat(toPath, &st, get_comp(L)) == 0) {
        F.free(fromPath);
        F.free(toPath);
        luaL_error(L, "File exists");
    }
    if (mount1->flags & MOUNT_FLAG_RO) res = copy_mmfs(mount1->root_dir, mmfs_traverse(mount1->root_dir, fromPath), toPath, get_comp(L));
    else res = copy_fs(fromPath, toPath, get_comp(L));
    F.free(fromPath);
    F.free(toPath);
    if (res != 0) err(L, 1, "Could not copy file");
    return 0;
}

static int remove_all(const char * path, craftos_machine_t machine) {
    struct craftos_stat st;
    if (F.stat(path, &st, machine) != 0) return 0;
    if (S_ISDIR(st.st_mode)) {
        int res;
        char* newpath;
        craftos_DIR* dir;
        struct craftos_dirent * ent;
        dir = F.opendir(path, machine);
        if (dir == NULL) return -1;
        while ((ent = F.readdir(dir, machine)) != NULL) {
            path = F.malloc(strlen(path) + ent->d_namlen + 2); 
            strcpy(newpath, path);
            strcat(newpath, "/");
            strcat(newpath, ent->d_name);
            if ((res = remove_all(newpath, machine)) != 0) {
                F.free(newpath);
                return res;
            }
            F.free(newpath);
        }
        F.closedir(dir, machine);
    }
    return F.remove(path, machine);
}

static int fs_delete(lua_State *L) {
    const struct craftos_mount_list * mount;
    int res;
    const char * str = luaL_checkstring(L, 1);
    char* path = fixpath(get_comp(L), str, 1, 1, &mount);
    if (path == NULL) return 0;
    if (*path == 0 || (path[0] == '/' && path[1] == 0)) {
        F.free(path);
        luaL_error(L, "Cannot delete mount");
    }
    if (mount->flags & MOUNT_FLAG_RO) err(L, 1, "Permission denied");
    if ((res = remove_all(path, get_comp(L))) != 0) {
        F.free(path);
        err(L, 1, strerror(-res));
    }
    F.free(path);
    return 0;
}

static int fs_combine(lua_State *L) {
    const char * str = luaL_checkstring(L, 1);
    char* basePath = F.malloc(strlen(str) + 1);
    int i;
    strcpy(basePath, str);
    for (i = 2; i <= lua_gettop(L); i++) {
        if (!lua_isstring(L, i)) {
            F.free(basePath);
            luaL_argerror(L, i, "expected string");
        }
        str = lua_tostring(L, i);
        if (*str != 0) {
            if (str[0] == '/' || str[0] == '\\') str++;
            basePath = F.realloc(basePath, strlen(basePath) + strlen(str) + 2);
            strcat(basePath, "/");
            strcat(basePath, str);
            /* TODO: fix trailing slashes */
        }
    }
    char* norm = normalizePath(basePath, 1);
    F.free(basePath);
    lua_pushstring(L, norm);
    F.free(norm);
    return 1;
}

static int fs_open(lua_State *L) {
    const struct craftos_mount_list * mount;
    craftos_machine_t computer = get_comp(L);
    const char * mode = luaL_checkstring(L, 2);
    if (
        (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') ||
        (
            !(mode[1] == '+' && mode[2] == 'b' && mode[3] == '\0') &&
            !(mode[1] == '+' && mode[2] == '\0') &&
            !(mode[1] == 'b' && mode[2] == '\0') &&
            mode[1] != '\0'
        )) luaL_error(L, "%s: Unsupported mode", mode);
    const char * str = luaL_checkstring(L, 1);
    char* path = mode[0] == 'r' ? fixpath(get_comp(L), str, 1, 1, &mount) : fixpath_mkdir(get_comp(L), str, 1, &mount);
    if (path == NULL) {
        char* p = fixpath(computer, str, 0, 0, NULL);
        lua_pushnil(L);
        if (mode[0] != 'r' && fixpath_ro(computer, str))
            lua_pushfstring(L, "/%s: Access denied", p);
        else lua_pushfstring(L, "/%s: No such file", p);
        F.free(p);
        return 2;
    }
    if (computer->files_open >= 128) {
        F.free(path);
        err(L, 1, "Too many files already open");
    }
    int fpid;
    if (mount->flags & MOUNT_FLAG_MMFS) {
        if (mode[0] != 'r') {
            char* p = fixpath(computer, str, 0, 0, NULL);
            lua_pushnil(L);
            lua_pushfstring(L, "/%s: Access denied", p);
            F.free(p);
            F.free(path);
            return 2;
        }
        const struct mmfs_dir_ent * ent = mmfs_traverse(mount->root_dir, path);
        if (ent == NULL) {
            char* p = fixpath(computer, str, 0, 0, NULL);
            lua_pushnil(L);
            lua_pushfstring(L, "/%s: No such file", p);
            F.free(p);
            F.free(path);
            return 2;
        }
        if (ent->is_dir) {
            char* p = fixpath(computer, str, 0, 0, NULL);
            lua_pushnil(L);
            if (strchr(mode, 'r') != NULL) lua_pushfstring(L, "/%s: Not a file", p);
            else lua_pushfstring(L, "/%s: Cannot write to directory", p);
            F.free(p);
            F.free(path);
            return 2; 
        }
        struct mmfs_fd ** fp = (struct mmfs_fd**)lua_newuserdata(L, sizeof(struct mmfs_fd*));
        fpid = lua_gettop(L);
        *fp = F.malloc(sizeof(struct mmfs_fd));
        (*fp)->start = (*fp)->ptr = mmfs_get_data(mount->root_dir, ent);
        (*fp)->end = (*fp)->start + ent->size;

        lua_createtable(L, 0, 1);
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_gc, 1);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);

        lua_createtable(L, 0, 4);
        lua_pushstring(L, "close");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_close, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "seek");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_seek, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "read");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_readByte, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "readAll");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_readAllByte, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "readLine");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_mmfs_readLine, 1);
        lua_settable(L, -3);
    } else {
        struct craftos_stat st;
        if (F.stat(path, &st, computer) != 0 && mode[0] == 'r') {
            char* p = fixpath(computer, str, 0, 0, NULL);
            lua_pushnil(L);
            lua_pushfstring(L, "/%s: No such file", p);
            F.free(p);
            F.free(path);
            return 2;
        } else if (S_ISDIR(st.st_mode)) { 
            char* p = fixpath(computer, str, 0, 0, NULL);
            lua_pushnil(L);
            if (strchr(mode, 'r') != NULL) lua_pushfstring(L, "/%s: Not a file", p);
            else lua_pushfstring(L, "/%s: Cannot write to directory", p);
            F.free(p);
            F.free(path);
            return 2; 
        }
        FILE ** fp = (FILE**)lua_newuserdata(L, sizeof(FILE*));
        fpid = lua_gettop(L);
        *fp = F.fopen(path, mode, computer);
        if (*fp == NULL) {
            int ok = 0;
            if (strchr(mode, 'a')) {
                *fp = F.fopen(path, "wb", computer);
                ok = *fp != NULL;
            }
            if (!ok) {
                char* p = fixpath(computer, str, 0, 0, NULL);
                lua_remove(L, fpid);
                lua_pushnil(L);
                lua_pushfstring(L, "/%s: No such file", p);
                F.free(p);
                F.free(path);
                return 2;
            }
        }

        lua_createtable(L, 0, 1);
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_gc, 1);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);

        lua_createtable(L, 0, 4);
        lua_pushstring(L, "close");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_close, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "seek");
        lua_pushvalue(L, fpid);
        lua_pushcclosure(L, fs_handle_seek, 1);
        lua_settable(L, -3);

        if (mode[0] == 'r' || strchr(mode, '+')) {
            lua_pushstring(L, "read");
            lua_pushvalue(L, fpid);
            lua_pushcclosure(L, fs_handle_readByte, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "readAll");
            lua_pushvalue(L, fpid);
            lua_pushcclosure(L, fs_handle_readAllByte, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "readLine");
            lua_pushvalue(L, fpid);
            lua_pushcclosure(L, fs_handle_readLine, 1);
            lua_settable(L, -3);
        }

        if (mode[0] == 'w' || mode[0] == 'a' || strchr(mode, '+')) {
            if (strchr(mode, 'b')) {
                lua_pushstring(L, "write");
                lua_pushvalue(L, fpid);
                lua_pushcclosure(L, fs_handle_writeByte, 1);
                lua_settable(L, -3);
            } else {
                lua_pushstring(L, "write");
                lua_pushvalue(L, fpid);
                lua_pushcclosure(L, fs_handle_writeString, 1);
                lua_settable(L, -3);
            }

            lua_pushstring(L, "writeLine");
            lua_pushvalue(L, fpid);
            lua_pushcclosure(L, fs_handle_writeLine, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "flush");
            lua_pushvalue(L, fpid);
            lua_pushcclosure(L, fs_handle_flush, 1);
            lua_settable(L, -3);
        }
    }
    computer->files_open++;
    F.free(path);
    return 1;
}

static int fs_getDir(lua_State *L) {
    char* path = normalizePath(luaL_checkstring(L, 1), 1);
    if (path[0] == 0 || (path[0] == '/' && path[1] == 0)) {
        F.free(path);
        lua_pushliteral(L, "..");
        return 1;
    }
    char* slash = strrchr(path, '/');
    if (slash == NULL) lua_pushliteral(L, "");
    else lua_pushlstring(L, path, slash - path);
    return 1;
}

#define st_time_ms(st) ((st##tim.tv_nsec / 1000000) + (st##time * 1000))

static int fs_attributes(lua_State *L) {
    const struct craftos_mount_list * mount;
    const char * str = luaL_checkstring(L, 1);
    char* path = fixpath(get_comp(L), str, 1, 1, &mount);
    if (path == NULL) err(L, 1, "No such file");
    if (mount->flags & MOUNT_FLAG_MMFS) {
        const struct mmfs_dir_ent * d = mmfs_traverse(mount->root_dir, path);
        if (d == NULL) {
            F.free(path);
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, 0, 6);
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "modification");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "modified");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "created");
        lua_pushinteger(L, d->is_dir ? 0 : d->size);
        lua_setfield(L, -2, "size");
        lua_pushboolean(L, d->is_dir);
        lua_setfield(L, -2, "isDir");
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, "isReadOnly");
    } else {
        struct craftos_stat st;
        if (F.stat(path, &st, get_comp(L)) != 0) {
            F.free(path);
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, 0, 6);
        lua_pushinteger(L, st_time_ms(st.st_m));
        lua_setfield(L, -2, "modification");
        lua_pushinteger(L, st_time_ms(st.st_m));
        lua_setfield(L, -2, "modified");
        lua_pushinteger(L, st_time_ms(st.st_c));
        lua_setfield(L, -2, "created");
        lua_pushinteger(L, S_ISDIR(st.st_mode) ? 0 : st.st_size);
        lua_setfield(L, -2, "size");
        lua_pushboolean(L, S_ISDIR(st.st_mode));
        lua_setfield(L, -2, "isDir");
        if (mount->flags & MOUNT_FLAG_RO) lua_pushboolean(L, 1);
        else lua_pushboolean(L, F.access(path, W_OK, get_comp(L)) != 0);
        lua_setfield(L, -2, "isReadOnly");
    }
    F.free(path);
    return 1;
}

static int fs_getCapacity(lua_State *L) {
    const struct craftos_mount_list * mount;
    const char * str = luaL_checkstring(L, 1);
    char* path = fixpath(get_comp(L), str, 0, 1, &mount);
    if (path == NULL) luaL_error(L, "%s: Invalid path", str);
    if (mount->flags & MOUNT_FLAG_MMFS) {
        F.free(path);
        lua_pushnil(L);
        return 1;
    }
    struct craftos_statvfs st;
    if (F.statvfs(path, &st, get_comp(L)) != 0) {
        F.free(path);
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, st.f_blocks * st.f_bsize);
    F.free(path);
    return 1;
}

const luaL_Reg fs_lib[] = {
    {"list", fs_list},
    {"exists", fs_exists},
    {"isDir", fs_isDir},
    {"isReadOnly", fs_isReadOnly},
    {"getName", fs_getName},
    {"getDrive", fs_getDrive},
    {"getSize", fs_getSize},
    {"getFreeSpace", fs_getFreeSpace},
    {"makeDir", fs_makeDir},
    {"move", fs_move},
    {"copy", fs_copy},
    {"delete", fs_delete},
    {"combine", fs_combine},
    {"open", fs_open},
    {"getDir", fs_getDir},
    {"attributes", fs_attributes},
    {"getCapacity", fs_getCapacity},
    {NULL, NULL}
};
