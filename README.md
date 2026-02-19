# craftos-base
A common library implementing a basic ComputerCraft emulator in ANSI C. For use in emulators running on all sorts of systems.

## Usage
This library is meant to be included into a platform-specific application, which implements the required functions for input/output and other non-portable components.

Include the library using CMake:
```cmake
add_subdirectory(craftos-base)
target_link_libraries(project INTERFACE craftos-base)
```

You must call `craftos_init` before using craftos-base. This takes a structure of function pointers which implement OS routines. Most of the entries are optional for basic functionality, but some are required, so make sure those are implemented. Initialize the structure with `NULL`s before use.

Then use `craftos_machine_create` to create a new emulator instance, passing a configuration structure with construction options. Afterward, repeatedly call `craftos_machine_run` on the machine to run the emulator. Call `craftos_machine_destroy` to destroy it.

Between calls you may use `craftos_machine_queue_event` or any of the `craftos_event_*` calls to push events into the system. This is required to keep the system going. You can also call `craftos_terminal_render` to render the terminal to a framebuffer, which supports various scales (X and Y independent) and pixel formats through the `convertPixelValue` OS function and `depth` parameter.

The emulator supports a few extension abilities to provide extra functionality:
- Use `craftos_machine_add_api` to add global APIs to the computer, which will be loaded at startup like any other API.
- Use `craftos_machine_mount_{real|mmfs}` to mount extra directories into the CC filesystem. This can either reference a filesystem path (which uses the filesystem routines in the function table) or a read-only [mmfs](https://gist.github.com/MCJack123/a89ce2f9847989940667620257b597a6)-formatted memory block, the latter of which is useful in embedded systems with a lack of a real filesystem.
- Use `craftos_machine_peripheral_attach` to attach a peripheral to a side. This can be used to emulate special peripheral types. See the function's docs for more information on how this works.
- The `craftos_tmpfs.h` header provides a custom in-memory filesystem routine set which can be used on systems which lack working `stdio.h` functionality.

See `include/craftos.h` for a full list of all OS functions that can be implemented. See `example/craftos-lite.c` for a working example using SDL3, libcurl and POSIX as a backend.

## License
craftos-base is licensed under the MIT license.
