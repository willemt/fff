
#include <stdio.h>
#include <uv.h>
#include "filewatcher.h"

int file_added(
    char* name,
    int is_dir,
    unsigned int size,
    unsigned long mtime)
{
    printf("added: %s %dB %d\n", name, size, is_dir);
    return 0;
}

int file_removed(char* name)
{
    printf("removed: %s\n", name);
    return 0;
}

int file_changed(char* name, int new_size, unsigned long mtime)
{
    printf("changed: %s %d\n", name, new_size);
    return 0;
}

int file_moved(char* name, char* new_name, unsigned long mtime)
{
    printf("moved: %s %s\n", name, new_name);
    return 0;
}

static void __periodic(uv_timer_t* handle, int status)
{
    printf("%d\n", status);
}


int main()
{
    uv_loop_t* loop;

    loop = uv_default_loop();

    uv_timer_t *periodic_req;
    periodic_req = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, periodic_req);
    uv_timer_start(periodic_req, __periodic, 0, 1000);

    filewatcher_new(".", loop,
            &((filewatcher_cbs_t){
                .file_added = file_added, 
                .file_removed = file_removed, 
                .file_changed = file_changed, 
                .file_moved = file_moved
                }));
    
    uv_run(loop, UV_RUN_DEFAULT);

    return 1;
}
