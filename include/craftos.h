/**
 * craftos.h
 * craftos-base
 * 
 * This file declares all the functions required to implement a basic CraftOS
 * emulator.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#ifndef CRAFTOS_H
#define CRAFTOS_H
#include <stdarg.h>
#include <stdio.h>

#ifndef lauxlib_h
struct luaL_Reg; /* silences warning */
#endif
#ifndef lua_h
struct lua_State; /* silences warning */
#endif

struct craftos_mount_list;
struct craftos_peripheral_list;
struct craftos_alarm_list;
struct craftos_sw_timer_list;
struct craftos_api_list;

/** An opaque type used for iterating a directory. */
typedef void * craftos_DIR;

/** An opaque type used for HTTP response handles. */
typedef void * craftos_http_handle_t;

/** An opaque type used for WebSocket handles. */
typedef void * craftos_http_websocket_t;

/** An opaque type representing a mutex or spinlock. */
typedef void * craftos_mutex_t;

/** An enum containing statuses for the machine. */
typedef enum craftos_status {
    /** The computer has shut down normally. */
    CRAFTOS_MACHINE_STATUS_SHUTDOWN,
    /** The computer is currently running (not returned by `craftos_machine_run`). */
    CRAFTOS_MACHINE_STATUS_RUNNING,
    /** The computer requested a restart. */
    CRAFTOS_MACHINE_STATUS_RESTART,
    /** The computer has yielded for events. */
    CRAFTOS_MACHINE_STATUS_YIELD,
    /** The computer has thrown an error. */
    CRAFTOS_MACHINE_STATUS_ERROR
} craftos_status_t;

/** An enum containing sides for redstone interaction. */
typedef enum craftos_redstone_side {
    CRAFTOS_REDSTONE_SIDE_INVALID = -1,
    CRAFTOS_REDSTONE_SIDE_TOP = 0,
    CRAFTOS_REDSTONE_SIDE_BOTTOM,
    CRAFTOS_REDSTONE_SIDE_LEFT,
    CRAFTOS_REDSTONE_SIDE_RIGHT,
    CRAFTOS_REDSTONE_SIDE_FRONT,
    CRAFTOS_REDSTONE_SIDE_BACK,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_TOP,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_BOTTOM,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_LEFT,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_RIGHT,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_FRONT,
    CRAFTOS_REDSTONE_SIDE_BUNDLED_BACK
} craftos_redstone_side_t;

/** A structure representing a directory entry. */
typedef struct craftos_dirent {
	long d_ino;
	unsigned short d_reclen;
    unsigned char d_type;
	unsigned char d_namlen;
	char d_name[256];
} craftos_dirent;

/** A structure containing file time. */
struct craftos_timespec {
    unsigned long tv_sec;
    unsigned long tv_nsec;
};

/** A structure containing file info. */
struct craftos_stat {
    unsigned short st_dev;
    unsigned short st_ino;
    unsigned short st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    unsigned short st_rdev;
    unsigned long st_size;
    unsigned int st_blksize;
    unsigned long st_blocks;
    struct craftos_timespec st_atim;
    struct craftos_timespec st_ctim;
    struct craftos_timespec st_mtim;
};

/** A structure containing filesystem info. */
struct craftos_statvfs {
    unsigned long  f_bsize;    /* Filesystem block size */
    unsigned long  f_frsize;   /* Fragment size */
    unsigned long  f_blocks;   /* Size of fs in f_frsize units */
    unsigned long  f_bfree;    /* Number of free blocks */
    unsigned long  f_bavail;   /* Number of free blocks for
                                    unprivileged users */
    unsigned long  f_files;    /* Number of inodes */
    unsigned long  f_ffree;    /* Number of free inodes */
    unsigned long  f_favail;   /* Number of free inodes for
                                    unprivileged users */
    unsigned long  f_fsid;     /* Filesystem ID */
    unsigned long  f_flag;     /* Mount flags */
    unsigned long  f_namemax;  /* Maximum filename length */
};

/** A structure representing a key-value pair for HTTP headers. */
typedef struct craftos_http_header {
    const char * key;
    const char * value;
} craftos_http_header_t;

/** Holds all information relating to a terminal instance. */
typedef struct craftos_terminal {
    unsigned int width; /* The width of the terminal in characters */
    unsigned int height; /* The height of the terminal in characters */
    unsigned char * screen; /* The buffer storing the characters displayed on screen */
    unsigned char * colors; /* The buffer storing the foreground and background colors for each character */
    unsigned int palette[16]; /* The color palette for the computer */
    unsigned int pixelPalette[16]; /* The color palette for the computer, in pixel values */
    int cursorX; /* The X position of the cursor */
    int cursorY; /* The Y position of the cursor */
    unsigned char changed : 1; /* Whether the terminal's data has been changed and needs to be redrawn - the renderer will not update your changes until you set this! */
    unsigned char blink : 1; /* Whether the cursor is currently drawn on-screen */
    unsigned char canBlink : 1; /* Whether the cursor should blink */
    unsigned char paletteChanged : 1; /* Whether the palette needs updating */
    unsigned char cursorColor : 4; /* The color of the cursor */
    unsigned char activeColors; /* The active colors for terminal writing */
} * craftos_terminal_t;

/** Holds information to be used when setting up a computer. */
typedef struct craftos_machine_config {
    /** The ID for the computer. */
    int id;

    /** The initial label for the computer, if one is set. */
    const char * label;

    /**
     * A pointer to an [mmfs](https://gist.github.com/MCJack123/a89ce2f9847989940667620257b597a6) filesystem in memory for holding the ROM.
     * This may be used to store the ROM in flash separate from the real filesystem.
     * If unset, the ROM must be located at `/rom` on the filesystem.
     */
    const void * rom_mmfs;

    /** The BIOS to load on the machine. If unset, the BIOS must be located at `/rom/bios.lua` on the filesystem. */
    const char * bios;

    /** The width of the screen, in either characters or pixels. */
    unsigned int width;
    
    /** The height of the screen, in either characters or pixels. */
    unsigned int height;

    /**
     * If this is 0, the width/height parameters refer to characters.
     * If >0, the width/height parameters refer to pixels, and this value is the
     * scale the characters will be drawn at. The character size will be
     * automatically calculated from the screen size scaled by this value.
     */
    int size_pixel_scale;

    /** The path to the computer's storage prefix on the filesystem. If unset, the whole filesystem will be used (equivalent to `"/"`). */
    const char * base_path;
} craftos_machine_config_t;

/** Represents a computer machine instance. */
typedef struct craftos_machine {
    int id;
    craftos_status_t running;
    struct lua_State *L;
    craftos_terminal_t term;
    struct lua_State *eventQueue;
    craftos_mutex_t eventQueueLock;
    struct lua_State *coro;
    const char * bios;
    double system_start;
    unsigned char files_open;
    unsigned char openWebsockets;
    unsigned char requests_open;
    unsigned char pixel_scale;
    char * label;
    unsigned short rs_output[12];
    struct craftos_mount_list * mounts;
    struct craftos_peripheral_list * peripherals;
    union {
        struct craftos_alarm_list * alarms;
        struct craftos_sw_timer_list * timers;
    };
    struct craftos_api_list * apis;
} * craftos_machine_t;

/** Holds all the function pointers required by the implementation. */
typedef struct craftos_func {
    /* === REQUIRED FUNCTIONS === */

    /**
     * Returns the current time in UTC milliseconds. This is not required to be
     * synced with the real time, but it must be monotonic. This function returns
     * a double to ensure 32-bit systems can store a full timestamp.
     * @return The current system time in milliseconds since 1970-01-01
     */
    double (*timestamp)();

    /**
     * Converts an RGB color or palette index into a pixel value. This is used
     * in rendering the terminal. Only the lowest bits (as specified by the
     * depth parameter to `craftos_terminal_render`) are used in rendering.
     * @param index The index of the color
     * @param r The red value of the color
     * @param g The green value of the color
     * @param b The blue value of the color
     * @return The pixel value to draw to the framebuffer
     */
    unsigned long (*convertPixelValue)(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

    /* === CC FUNCTIONS === */

    /**
     * (Optional) Sets a timer to fire in the specified amount of time. If this
     * function is not implemented, timers will be handled in software through
     * `timestamp`.
     * @param millis The amount of time until firing, in milliseconds
     * @param machine The machine the timer is for
     * @return A unique ID for the timer (for cancellation)
     */
    int (*startTimer)(unsigned long millis, craftos_machine_t machine);

    /**
     * (Optional) Cancels a previously started timer.
     * @param id The ID of the timer to cancel
     * @param machine The machine the timer is for
     */
    void (*cancelTimer)(int id, craftos_machine_t machine);

    /**
     * (Optional) Tells the system to update the computer label in non-volatile
     * storage, if available.
     * @param label The label to set - may be NULL to unset the label
     * @param machine The machine to set the label for
     */
    void (*setComputerLabel)(const char * label, craftos_machine_t machine);

    /**
     * (Optional) Returns redstone input on a side.
     * @param side The side to check - 0-5 are normal analog (0-15), 6-11 are bundled (0-65535)
     * @param machine The machine to query for
     * @return The strength of the input from 0-15 (sides 0-5) or a bitmask of set colors (sides 6-11)
     */
    unsigned short (*redstone_getInput)(craftos_redstone_side_t side, craftos_machine_t machine);

    /**
     * (Optional) Sets redstone output on a side.
     * @param side The side to check - 0-5 are normal analog (0-15), 6-11 are bundled (0-65535)
     * @param value The strength of the output from 0-15 (sides 0-5) or a bitmask of set colors (sides 6-11)
     * @param machine The machine to query for
     */
    void (*redstone_setOutput)(craftos_redstone_side_t side, unsigned short value, craftos_machine_t machine);

    /* === OS FUNCTIONS === */

    /**
     * (Optional) Memory allocator. If set to NULL, this will use `malloc`.
     * @param size The size of the memory to allocate
     * @return A new memory block, or NULL on error
     */
    void * (*malloc)(size_t size);

    /**
     * (Optional) Memory reallocator. If set to NULL, this will use `realloc`.
     * @param ptr The pointer to reallocate
     * @param size The size of the memory to allocate
     * @return A resized memory block, or NULL on error
     */
    void * (*realloc)(void * ptr, size_t nsize);

    /**
     * (Optional) Memory deallocator. If set to NULL, this will use `free`.
     * @param ptr The pointer to free
     */
    void (*free)(void * ptr);

    /**
     * (Optional) Creates a new mutex. If not implemented, multithreading will
     * not be protected from race conditions.
     * @return A new mutex object
     */
    craftos_mutex_t (*mutex_create)();

    /**
     * (Optional) Destroys a previously created mutex.
     * @param mutex The mutex to destroy
     */
    void (*mutex_destroy)(craftos_mutex_t mutex);

    /**
     * (Optional) Locks a mutex, waiting forever.
     * @param mutex The mutex to lock
     * @return 0 on success, non-0 on error
     */
    int (*mutex_lock)(craftos_mutex_t mutex);

    /**
     * (Optional) Unlocks a mutex.
     * @param mutex The mutex to unlock
     */
    void (*mutex_unlock)(craftos_mutex_t mutex);

    /**
     * (Optional) An implementation of C `fopen` for machine use. If set to NULL,
     * standard `fopen` will be used instead.
     * @param file The file path to open
     * @param mode The mode to open the file in
     * @param machine The machine opening the file
     * @return The newly opened file handle, or NULL on error; errors should be set on `errno`
     */
    FILE * (*fopen)(const char * file, const char * mode, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fclose` for machine use. If set to NULL,
     * standard `fclose` will be used instead.
     * @param fp The file to close
     * @param machine The machine closing the file
     * @return 0 on success, EOF otherwise
     */
    int (*fclose)(FILE *fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fread` for machine use. If set to NULL,
     * standard `fread` will be used instead.
     * @param buf The buffer to read into
     * @param size The size of the elements in the buffer
     * @param count The number of elements in the buffer
     * @param fp The file pointer to read from
     * @param machine The machine operating on the file
     * @return The number of elements read in
     */
    size_t (*fread)(void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fwrite` for machine use. If set to NULL,
     * standard `fwrite` will be used instead.
     * @param buf The buffer to write to
     * @param size The size of the elements in the buffer
     * @param count The number of elements in the buffer
     * @param fp The file pointer to read from
     * @param machine The machine operating on the file
     * @return The number of elements written out
     */
    size_t (*fwrite)(const void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fflush` for machine use. If set to NULL,
     * standard `fflush` will be used instead.
     * @param fp The file pointer to flush from
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*fflush)(FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fgetc` for machine use. If set to NULL,
     * standard `fgetc` will be used instead.
     * @param fp The file pointer to read from
     * @param machine The machine operating on the file
     * @return The character read, or EOF on failure
     */
    int (*fgetc)(FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fputc` for machine use. If set to NULL,
     * standard `fputc` will be used instead.
     * @param ch The character to write
     * @param fp The file pointer to write to
     * @param machine The machine operating on the file
     * @return `ch` on success, EOF on error
     */
    int (*fputc)(int ch, FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `ftell` for machine use. If set to NULL,
     * standard `ftell` will be used instead.
     * @param fp The file pointer to seek in
     * @param machine The machine operating on the file
     * @return The position in the file, or -1 on error
     */
    long (*ftell)(FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `fseek` for machine use. If set to NULL,
     * standard `fseek` will be used instead.
     * @param fp The file pointer to seek in
     * @param offset The offset to adjust by
     * @param origin The origin of the new pointer (`SEEK_*`)
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*fseek)(FILE * fp, long offset, int origin, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `feof` for machine use. If set to NULL,
     * standard `feof` will be used instead.
     * @param fp The file pointer to check
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on EOF
     */
    int (*feof)(FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `ferror` for machine use. If set to NULL,
     * standard `ferror` will be used instead.
     * @param fp The file pointer to check
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*ferror)(FILE * fp, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `remove` for machine use. If set to NULL,
     * standard `remove` will be used instead.
     * @param path The path to delete
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*remove)(const char * path, craftos_machine_t machine);

    /**
     * (Optional) An implementation of C `rename` for machine use. If set to NULL,
     * standard `rename` will be used instead.
     * @param from The path to the file to move
     * @param to The path to move the file into
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*rename)(const char * from, const char * to, craftos_machine_t machine);

    /**
     * An implementation of POSIX `mkdir` for machine use. If set to NULL, and a
     * compatible `mkdir` is available, it will be used instead. Otherwise,
     * `craftos_init` will fail.
     * @param from The path to the directory to create
     * @param mode The mode for the directory (unused, always 0777)
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*mkdir)(const char * path, int mode, craftos_machine_t machine);

    /**
     * An implementation of POSIX `access` for machine use. If set to NULL, and
     * a compatible `access` is available, it will be used instead. Otherwise,
     * `craftos_init` will fail.
     * @param from The path to the directory to create
     * @param mode The flags to use (usually W_OK = 2)
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*access)(const char * path, int flags, craftos_machine_t machine);

    /**
     * An implementation of POSIX `stat` for machine use. If set to NULL, and a
     * compatible `stat` is available, it will be used instead. Otherwise,
     * `craftos_init` will fail.
     * @param path The path to the file to query
     * @param st The structure to fill with info
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*stat)(const char * path, struct craftos_stat * st, craftos_machine_t machine);

    /**
     * An implementation of POSIX `statvfs` for machine use. If set to NULL, and
     * a compatible `statvfs` is available, it will be used instead. Otherwise,
     * `craftos_init` will fail.
     * @param path The path to a file on the filesystem to query
     * @param st The structure to fill with info
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*statvfs)(const char * path, struct craftos_statvfs * st, craftos_machine_t machine);

    /**
     * An implementation of dirent `opendir` for machine use. This may simply
     * be a wrapper around `opendir`. If set to NULL, and a compatible `opendir`
     * is available, it will be used instead. Otherwise, `craftos_init` will fail.
     * @param path The path to the directory to open
     * @param machine The machine opening the directory
     * @return A directory structure on success, or `NULL` on error
     */
    craftos_DIR * (*opendir)(const char * path, craftos_machine_t machine);

    /**
     * An implementation of dirent `closedir` for machine use. This may simply
     * be a wrapper around `closedir`. If set to NULL, and a compatible
     * `closedir` is available, it will be used instead. Otherwise,
     * `craftos_init` will fail.
     * @param dir The directory to close
     * @param machine The machine opening the directory
     * @return 0 on success, non-0 on error
     */
    int (*closedir)(craftos_DIR * dir, craftos_machine_t machine);

    /**
     * An implementation of dirent `readdir` for machine use. Be careful about
     * structure order - though `craftos_dirent` is in the same order as glibc,
     * it likely will not match other dirent implementations, so the members
     * will need to be copied into a new structure. Consider reentrancy if it is
     * a concern (could other threads be running computers concurrently?).
     * If set to NULL, and a compatible `readdir` is available, it will be used
     * instead. Otherwise, `craftos_init` will fail.
     * @param dir The directory being read
     * @param machine The machine opening the directory
     * @return A directory entry on success, or `NULL` on error
     */
    struct craftos_dirent * (*readdir)(craftos_DIR * dir, craftos_machine_t machine);

    /* === HTTP FUNCTIONS === */

    /**
     * Schedules an HTTP request. This function should run asynchronously where
     * possible. The application must call either `craftos_event_http_success`
     * or `craftos_event_http_failure` upon resolution of the request.
     * 
     * If this function is not implemented, HTTP will not be available in the
     * machines.
     * 
     * @param url The URL to request
     * @param method The method for the request
     * @param body The body to send, or NULL for no body
     * @param body_size The size of the body, or 0 for no body
     * @param headers A NULL-terminated list of headers
     * @param redirect Whether the application should automatically handle redirects
     * @param machine The machine making the request
     * @param 0 on success, non-0 on error
     */
    int (*http_request)(
        const char * url,
        const char * method,
        const unsigned char * body,
        size_t body_size,
        craftos_http_header_t * headers,
        int redirect,
        craftos_machine_t machine
    );

    /**
     * An implementation of `fclose` for HTTP handles. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param handle The handle to close
     * @param machine The machine closing the file
     * @return 0 on success, EOF otherwise
     */
    int (*http_handle_close)(craftos_http_handle_t handle, craftos_machine_t machine);

    /**
     * An implementation of `fread` for HTTP handles. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param buf The buffer to read into
     * @param size The size of the elements in the buffer
     * @param count The number of elements in the buffer
     * @param handle The handle to read from
     * @param machine The machine operating on the file
     * @return The number of elements read in
     */
    size_t (*http_handle_read)(void * buf, size_t size, size_t count, craftos_http_handle_t handle, craftos_machine_t machine);

    /**
     * An implementation of `fgetc` for HTTP handles. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param handle The handle to read from
     * @param machine The machine operating on the file
     * @return The character read, or EOF on failure
     */
    int (*http_handle_getc)(craftos_http_handle_t handle, craftos_machine_t machine);

    /**
     * An implementation of `ftell` for HTTP handles. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param handle The handle to seek in
     * @param machine The machine operating on the file
     * @return The position in the file, or -1 on error
     */
    long (*http_handle_tell)(craftos_http_handle_t handle, craftos_machine_t machine);

    /**
     * An implementation of `fseek` for HTTP handles. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param handle The handle to seek in
     * @param offset The offset to adjust by
     * @param origin The origin of the new pointer (`SEEK_*`)
     * @param machine The machine operating on the file
     * @return 0 on success, non-0 on error
     */
    int (*http_handle_seek)(craftos_http_handle_t handle, long offset, int origin, craftos_machine_t machine);

    /**
     * Returns the response code for an HTTP handle. If this function is not
     * implemented, HTTP will not be available in the machines.
     * @param handle The handle to check
     * @param machine The machine operating on the handle
     * @return The HTTP response code
     */
    int (*http_handle_getResponseCode)(craftos_http_handle_t handle, craftos_machine_t machine);

    /**
     * Returns the next (or first) HTTP header from a previous header. This will
     * first be called with a pointer containing `NULL` to get the first header,
     * which is filled into the pointer passed; the result will then be passed
     * pack until there are no more headers, in which case this function should
     * fill in `NULL` to indicate no more headers remain.
     * 
     * If the headers are already in a compatible structure, this function may
     * simply advance pointers in the structure. Otherwise, the function may
     * allocate a new structure and repeatedly refresh it, freeing when the end
     * of the list is reached (the old pointer will not be used after calling).
     * 
     * If this function is not implemented, HTTP will not be available in the
     * machines.
     * 
     * @param handle The handle to query
     * @param header A double pointer to the previous header to get the next header from (inout)
     * @param machine The machine operating on the handle
     */
    void (*http_handle_getResponseHeader)(craftos_http_handle_t handle, craftos_http_header_t ** header, craftos_machine_t machine);

    /**
     * Schedules a WebSocket request. This function should run asynchronously
     * where possible. The application must call either
     * `craftos_event_websocket_success` or `craftos_event_websocket_failure`
     * upon resolution of the request.
     * 
     * If this function is not implemented, WebSockets will not be available in
     * the machines.
     * 
     * @param url The URL to connect to
     * @param headers A NULL-terminated list of headers
     * @param machine The machine making the request
     * @param 0 on success, non-0 on error
     */
    int (*http_websocket)(const char * url, craftos_http_header_t * headers, craftos_machine_t machine);

    /**
     * Sends a message on a WebSocket. This should be able to handle if the
     * socket was closed on the other end.
     * 
     * If this function is not implemented, WebSockets will not be available in
     * the machines.
     * 
     * @param handle The handle to send on
     * @param message_len The length of the message
     * @param message The message to send
     * @param binary Whether this is a binary message
     * @param machine The machine operating on the handle
     */
    void (*http_websocket_send)(craftos_http_websocket_t handle, size_t message_len, const unsigned char * message, int binary, craftos_machine_t machine);

    /**
     * Closes a WebSocket handle. This will only be called on a valid handle.
     * 
     * If this function is not implemented, WebSockets will not be available in
     * the machines.
     * 
     * @param handle The handle to close
     * @param machine The machine operating on the handle
     */
    void (*http_websocket_close)(craftos_http_websocket_t handle, craftos_machine_t machine);
} * craftos_func_t;

/**
 * Initializes the emulator with platform-dependent functions. This must be
 * called before any other craftos-base function.
 * @param functions A table of functions filled in by the application
 * @return 0 on success, non-0 on error
 */
extern int craftos_init(const craftos_func_t functions);

/**
 * Creates a new machine using the specified configuration.
 * @param config The configuration for the new machine
 * @return The new machine object, or NULL on error; destroy with `craftos_machine_destroy`
 */
extern craftos_machine_t craftos_machine_create(const craftos_machine_config_t * config);

/**
 * Destroys a previously created machine.
 * @param machine The machine to destroy
 */
extern void craftos_machine_destroy(craftos_machine_t machine);

/**
 * Runs the machine, handling startup and shutdown if necessary, and processing
 * events until the queue is empty. Call this in a loop alongside event-producing
 * code to run the computer.
 * 
 * If this function returns `CRAFTOS_MACHINE_STATUS_YIELD`, the computer has run
 * out of events in the queue. Use the time to process the rest of the system
 * and queue new events as they appear, then call the function again to resume.
 * 
 * If this function returns `CRAFTOS_MACHINE_STATUS_SHUTDOWN` or
 * `CRAFTOS_MACHINE_STATUS_RESTART`, the computer is now shut down and the Lua
 * state and queue are shut down. The two statuses are functionally equivalent,
 * but they can be used to handle the conditions in hardware.
 * 
 * If this function returns `CRAFTOS_MACHINE_STATUS_ERROR`, the computer threw a
 * BIOS error during execution. This is a similar state to shutdown, but
 * displays text on the screen describing the error. The machine can be run
 * again to restart the computer, or the system can choose to hang to display
 * the error.
 * 
 * @param machine The machine to run
 * @return The outcome of running the machine
 */
extern craftos_status_t craftos_machine_run(craftos_machine_t machine);

/**
 * Queues an arbitrary event in the machine.
 * 
 * The format parameter specifies the type of each parameter, which follows the
 * [Lua `string.pack` format](https://www.lua.org/manual/5.3/manual.html#6.4.2),
 * except for endian/alignment parameters, padding, and arbitrary size integers.
 * The `s` parameter will instead pull a `size_t` argument for size first, then
 * the string from the next argument - arbitrary integer sizes are not supported.
 * Additionally, the following parameters are available:
 * - `x`: `nil`
 * - `q`: boolean (encoded as `int`)
 * - `c`: single character string in `char`
 * - `F`: `lua_CFunction`
 * - `u`: light userdata
 * - `p`: full userdata pointer (takes `void*`, makes `void**` userdata)
 * - `r`: `luaL_Reg*` with functions to put in a table onto the queue
 * - `R`: `luaL_Reg*` with functions to put in a table onto the queue, consuming
 *        an upvalue pushed before this
 * For pointer types (`zsFR`), `NULL` values will be converted to `nil`.
 * 
 * This function is not safe to call in interrupts, as it interacts with the Lua
 * state, which may be running when the interrupt fires. If mutexes are
 * implemented on a single thread system, the mutex may deadlock in this case.
 * Use an auxiliary queue and empty it between `craftos_machine_run` calls if
 * this is a concern.
 * 
 * @param machine The machine to queue on
 * @param event The name of the event to queue (copied)
 * @param fmt The format for the parameters
 * @param ... The parameters for the event
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_queue_event(craftos_machine_t machine, const char * event, const char * fmt, ...);

/**
 * Adds an API to be loaded automatically on boot.
 * @param machine The machine to add to
 * @param name The name of the API global (not copied!)
 * @param funcs The functions in the API
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_add_api(craftos_machine_t machine, const char * name, const struct luaL_Reg * funcs);

/**
 * Mounts a filesystem directory inside the machine's filesystem.
 * Up to 8 mounts may be made to the same path - the order of mounting
 * determines search order for existing files/folders.
 * @param machine The machine to mount to
 * @param src The source filesystem path to mount (copied)
 * @param dest The destination path in the machine's filesystem (copied)
 * @param readOnly Whether the mount is read-only
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_mount_real(craftos_machine_t machine, const char * src, const char * dest, int readOnly);

/**
 * Mounts a read-only mmfs filesystem in memory inside the machine's filesystem.
 * Up to 8 mounts may be made to the same path - the order of mounting
 * determines search order for existing files/folders.
 * @param machine The machine to mount to
 * @param src The source filesystem to mount (not copied!)
 * @param dest The destination path in the machine's filesystem (copied)
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_mount_mmfs(craftos_machine_t machine, const void * src, const char * dest);

/**
 * Unmounts all previously mounted filesystems at a path.
 * @param machine The machine to unmount from
 * @param path The machine path to the filesystem(s) to unmount
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_unmount(craftos_machine_t machine, const char * path);

/**
 * Attaches a peripheral to a side.
 * @param machine The machine to attach to
 * @param side The side to attach on (not copied!)
 * @param types A NULL-terminated array of types for the peripheral (not copied!)
 * @param funcs The functions the peripheral exposes - `userp` will be passed as the first argument in a light userdata, then the function arguments follow (not copied!)
 * @param userp A pointer to pass to peripheral functions to store peripheral data
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_peripheral_attach(craftos_machine_t machine, const char * side, const char ** types, const struct luaL_Reg * funcs, void * userp);

/**
 * Detaches a previously attached peripheral.
 * @param machine The machine to detach from
 * @param side The side to detach
 * @return 0 on success, non-0 on error
 */
extern int craftos_machine_peripheral_detach(craftos_machine_t machine, const char * side);

/**
 * Renders a terminal to a framebuffer.
 * @param term The terminal to render
 * @param framebuffer The memory region for the framebuffer
 * @param stride The number of bytes in each line in the framebuffer
 * @param depth The bit depth for the screen (must be one of 4, 8, 16, 24, 32)
 * @param scaleX The scale to render the text at in the X direction
 * @param scaleY The scale to render the text at in the Y direction
 */
extern void craftos_terminal_render(craftos_terminal_t term, void * framebuffer, size_t stride, int depth, int scaleX, int scaleY);

/**
 * Renders a terminal to a 4-bit planar VGA framebuffer.
 * @param term The terminal to render
 * @param framebuffer The memory region for the framebuffer
 * @param stride The number of bytes in each line in the framebuffer
 * @param height The height of the framebuffer in lines
 */
extern void craftos_terminal_render_vga4p(craftos_terminal_t term, void * framebuffer, size_t stride, size_t height);

/**
 * Draws text to a terminal.
 * @param term The terminal to draw to
 * @param x The X position to write to
 * @param y The Y position to write to
 * @param text The text to write
 * @param len The length of the text
 * @param colors The color pair to use while writing (top nibble = BG, bottom = FG)
 */
extern void craftos_terminal_write(craftos_terminal_t term, int x, int y, const char * text, int len, unsigned char colors);

/**
 * Draws text to a terminal.
 * @param term The terminal to draw to
 * @param x The X position to write to
 * @param y The Y position to write to
 * @param text The text to write
 * @param colors The color pair to use while writing (top nibble = BG, bottom = FG)
 */
#define craftos_terminal_write_string(term, x, y, text, colors) craftos_terminal_write(term, x, y, text, strlen(text), colors)

/**
 * Draws literal text to a terminal.
 * @param term The terminal to draw to
 * @param x The X position to write to
 * @param y The Y position to write to
 * @param text The text to write
 * @param colors The color pair to use while writing (top nibble = BG, bottom = FG)
 */
#define craftos_terminal_write_literal(term, x, y, text, colors) craftos_terminal_write(term, x, y, text, sizeof(text)-1, colors)

/**
 * Fires a timer on the computer.
 * @param machine The machine the timer is for
 * @param id The ID of the timer
 */
extern void craftos_event_timer(craftos_machine_t machine, int id);

/**
 * Triggers a key press, including the corresponding `char` event if required.
 * @param machine The machine to trigger on
 * @param keycode The (old) CC keycode to trigger
 * @param repeat Whether this is a repeated key
 */
extern void craftos_event_key(craftos_machine_t machine, unsigned char keycode, int repeat);

/**
 * Triggers a key release.
 * @param machine The machine to trigger on
 * @param keycode The (old) CC keycode to trigger
 */
extern void craftos_event_key_up(craftos_machine_t machine, unsigned char keycode);

/**
 * Triggers a character typed.
 * @param machine The machine to trigger on
 * @param ch The character typed
 */
#define craftos_event_char(machine, ch) craftos_machine_queue_event(machine, "char", "c", ch)

/* Note: Mouse coordinates are given in character coordinates (1-based) if the
   pixel scale passed in the machine config is 0; otherwise, the coordinates
   should be in screen space (0-based), and the coordinates will be converted to
   character coordinates automatically based on the scale. */

/**
 * Triggers a mouse button press.
 * @param machine The machine to trigger on
 * @param button The button that was pressed
 * @param x The X coordinate of the mouse in the space determined by config->size_pixel_scale
 * @param Y The Y coordinate of the mouse in the space determined by config->size_pixel_scale
 */
extern void craftos_event_mouse_click(craftos_machine_t machine, int button, unsigned int x, unsigned int y);

/**
 * Triggers a mouse move. This will send a `mouse_drag` event if appropriate.
 * @param machine The machine to trigger on
 * @param x The X coordinate of the mouse in the space determined by config->size_pixel_scale
 * @param Y The Y coordinate of the mouse in the space determined by config->size_pixel_scale
 */
extern void craftos_event_mouse_move(craftos_machine_t machine, unsigned int x, unsigned int y);

/**
 * Triggers a mouse button release.
 * @param machine The machine to trigger on
 * @param button The button that was released
 * @param x The X coordinate of the mouse in the space determined by config->size_pixel_scale
 * @param Y The Y coordinate of the mouse in the space determined by config->size_pixel_scale
 */
extern void craftos_event_mouse_up(craftos_machine_t machine, int button, unsigned int x, unsigned int y);

/**
 * Triggers a mouse wheel scroll.
 * @param machine The machine to trigger on
 * @param button The direction of the mouse (1 = up, -1 = down)
 * @param x The X coordinate of the mouse in the space determined by config->size_pixel_scale
 * @param Y The Y coordinate of the mouse in the space determined by config->size_pixel_scale
 */
extern void craftos_event_mouse_scroll(craftos_machine_t machine, int direction, unsigned int x, unsigned int y);

/**
 * Triggers a terminate event. Note: Ctrl+T is handled automatically in software.
 * @param machine The machine to trigger on
 */
#define craftos_event_terminate(machine) craftos_machine_queue_event(machine, "terminate", "");

/**
 * Call this function to queue an event upon an HTTP response. This should be
 * called for any status code - failure codes will be converted to an 
 * `http_failure` event internally.
 * @param machine The machine to queue on
 * @param url The exact URL the response is for
 * @param handle An opaque pointer to be used for reading the HTTP response
 */
extern void craftos_event_http_success(craftos_machine_t machine, const char * url, craftos_http_handle_t handle);

/**
 * Call this function to queue an event upon an HTTP request failure.
 * @param machine The machine to queue on
 * @param url The exact URL the failure is for
 * @param err An error message to send in the event
 */
#define craftos_event_http_failure(machine, url, err) craftos_machine_queue_event(machine, "http_failure", "zz", url, err)

/**
 * Call this function to queue an event upon a successful WebSocket connection.
 * @param machine The machine to queue on
 * @param url The exact URL the response is for
 * @param handle An opaque pointer to be used for the WebSocket handle
 */
extern void craftos_event_websocket_success(craftos_machine_t machine, const char * url, craftos_http_websocket_t handle);

/**
 * Call this function to queue an event upon a WebSocket connection failure.
 * @param machine The machine to queue on
 * @param url The exact URL the failure is for
 * @param err An error message to send in the event
 */
#define craftos_event_websocket_failure(machine, url, err) craftos_machine_queue_event(machine, "websocket_failure", "zz", url, err)

/**
 * Call this function to queue an event upon an incoming WebSocket message.
 * @param machine The machine to queue on
 * @param url The exact URL the message is for
 * @param message_len The length of the message
 * @param message The message that was received
 * @param binary Whether the message is a binary message
 */
#define craftos_event_websocket_message(machine, url, message_len, message, binary) craftos_machine_queue_event(machine, "websocket_message", "zsq", url, (size_t)(message_len), message, binary)

/**
 * Call this function to queue an event upon a WebSocket closing.
 * @param machine The machine to queue on
 * @param url The exact URL the failure is for
 * @param message A message explaining why the socket closed
 * @param code The code for the closing, or -1 for abnormal close
 */
#define craftos_event_websocket_closed(machine, url, message, code) craftos_machine_queue_event(machine, "websocket_closed", (code) != -1 ? "zzi" : "zzx", url, message, (int)(code))

#endif
