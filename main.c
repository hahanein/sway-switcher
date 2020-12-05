#include <stdio.h>
#include <string.h>

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include "./xdg-shell.h"

struct wl_compositor *compositor;
struct wl_shm *shm;
struct xdg_wm_base *xdg_shell;

// registry_global_handler binds a few selected objects to the provided
// registry. These are:
// - Compositor (wl_compositor)
// - Shared memory buffers (wl_shm)
// - Desktop style user interfaces (xdg_wm_base)
void registry_global_handler(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
    {
        compositor = wl_registry_bind(registry, name,
                                      &wl_compositor_interface, 3);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        shm = wl_registry_bind(registry, name,
                               &wl_shm_interface, 1);
    }
    else if (strcmp(interface, "xdg_wm_base") == 0)
    {
        xdg_shell = wl_registry_bind(registry, name,
                                 &xdg_wm_base_interface, 1);
    }
}

void registry_global_remove_handler(
    void *data,
    struct wl_registry *registry,
    uint32_t name) {}

const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler};

void xdg_toplevel_configure_handler(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {
    printf("configure: %dx%d\n", width, height);
}

void xdg_toplevel_close_handler(
    void *data,
    struct xdg_toplevel *xdg_toplevel
) {
    printf("close\n");
}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler
};

void xdg_surface_configure_handler(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial
) {
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler
};

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(xdg_shell, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    wl_surface_commit(surface);

    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;

    // open an anonymous file and write zero bytes to it
    int fd = syscall(SYS_memfd_create, "buffer", 0);
    ftruncate(fd, size);

    // map anonymous file to memory
    unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // turn mapping into a shared memory pool
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    // allocate the buffer in the above pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

    // wait for surface to be configured
    wl_display_roundtrip(display);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    while (1)
    {
        wl_display_dispatch(display);
    }
}