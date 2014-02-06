
#include <stdio.h>
#include <uv.h>
#include "fff.h"

int file_added(
    void* udata,
    char* name,
    int is_dir,
    unsigned int size,
    unsigned long mtime)
{
    printf("added: %s %dB %d\n", name, size, is_dir);
    return 0;
}

static void __log(void *udata, void *src, const char *buf, ...)
{

    printf("%s\n", buf);

#if 0
    char stamp[32];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    int fd = (unsigned long) udata;
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
#endif
}

int file_removed(void* udata, char* name)
{
    printf("removed: %s\n", name);
    return 0;
}

int file_changed(void* udata, char* name, int new_size, unsigned long mtime)
{
    printf("changed: %s %d\n", name, new_size);
    return 0;
}

int file_moved(void* udata, char* name, char* new_name, unsigned long mtime)
{
    printf("moved: %s %s\n", name, new_name);
    return 0;
}

int file_scanned(void* udata, char* name)
{
    printf("scanned: %s\n", name);
    return 0;
}

static void __periodic(uv_timer_t* handle, int status)
{
    fff_periodic(handle->data,1000);
}

int main()
{
    uv_loop_t* loop;
    filewatcher_t* f;

    loop = uv_default_loop();

    f = fff_new(".", loop,
            &((filewatcher_cbs_t){
                .file_added = file_added, 
                .file_removed = file_removed, 
                .file_changed = file_changed, 
                .file_moved = file_moved,
                .file_scanned = file_scanned,
                .log = __log
                }), NULL);
    
    uv_timer_t *periodic_req;
    periodic_req = malloc(sizeof(uv_timer_t));
    periodic_req->data = f;
    uv_timer_init(loop, periodic_req);
    uv_timer_start(periodic_req, __periodic, 0, 1000);
    uv_run(loop, UV_RUN_DEFAULT);

    return 1;
}
