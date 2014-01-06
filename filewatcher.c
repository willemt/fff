
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <uv.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "linked_list_hashmap.h"
#include "filewatcher.h"

typedef struct file_s file_t;

typedef struct {
    char* path;
    uv_loop_t *loop;
    hashmap_t *files;
    filewatcher_cbs_t cb;
    void* cb_ctx;
} filewatcher_private_t;

struct file_s {
    char* name;
    unsigned int size;
    unsigned int mtime;
    int is_dir;
    void* udata;
};

int asprintf(char **resultp, const char *format, ...)
{
    char buf[1024];
    va_list args;

    va_start (args, format);
    vsprintf(buf, format, args);
    *resultp = strdup(buf);
    va_end (args);
    return 1;
}

static void __readdir_cb(uv_fs_t* req);
static void __stat_cb(uv_fs_t* req);
void __add_file(filewatcher_private_t* me, file_t* f);

#if 0
void __print_file(file_t* f, int depth)
{
    printf("%s %dB %llu\n", f->name, f->size, f->mtime);
}
#endif

static void __stat_cb(uv_fs_t* req)
{
    filewatcher_private_t* me;
    file_t* f;

    f = req->data;
    me = f->udata;

    f->size = req->statbuf.st_size;
    f->mtime = req->statbuf.st_mtim.tv_sec;
    if (req->statbuf.st_mode & S_IFDIR)
    {
        f->is_dir = 1;

        int r;
        uv_fs_t *req_rd;

        req_rd = calloc(1,sizeof(uv_fs_t));
        req_rd->data = me;

        if (0 != (r = uv_fs_readdir(me->loop, req_rd, f->name, 0,
                    __readdir_cb)))
        {
            printf("ERROR\n");
        }
    }

    __add_file(me, f);

    uv_fs_req_cleanup(req);
}

static void __readdir_cb(uv_fs_t* req)
{
    filewatcher_private_t* me;
    char* res;
    int i;

    me = req->data;
    res = req->ptr;

    for (i=0; i<req->result; i++)
    {
        int r;
        
        file_t *f;
        f = calloc(1,sizeof(file_t));
        asprintf(&f->name, "%s/%s", req->path, res);
        f->udata = me;

        /* stat() the file */
        uv_fs_t *stat_req;
        stat_req = malloc(sizeof(uv_fs_t));
        stat_req->data = f;
        if (0 != (r = uv_fs_stat(me->loop, stat_req, f->name, __stat_cb)))
        {
            printf("ERROR\n");
        }

        res += strlen(res) + 1;
    }

    uv_fs_req_cleanup(req);
}

void __add_file(filewatcher_private_t* me, file_t* f)
{
    hashmap_put(me->files, f->name, f);
    if (me->cb.file_added)
        me->cb.file_added(me->cb_ctx, f->name, f->is_dir, f->size, f->mtime);
}

/**
 * djb2 by Dan Bernstein. */
static unsigned long __hash_file(const void *obj)
{
    const char* str = (const char*)obj;
    unsigned long hash = 5381;
    int c;
    
    for (; (c = *str++);)
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static long __cmp_file(const void *obj, const void *other)
{
    return strcmp(obj,other);
}

filewatcher_t *filewatcher_new(
        char* directory,
        uv_loop_t* loop,
        filewatcher_cbs_t *cbs,
        void* cb_ctx)
{
    filewatcher_private_t* me;
    uv_fs_t *req;
    int r;

    me = calloc(1,sizeof(filewatcher_private_t));
    me->files = hashmap_new(__hash_file, __cmp_file, 11);
    me->loop = loop;
    memcpy(&me->cb, cbs, sizeof(filewatcher_cbs_t));

    /* start watching */
    req = calloc(1,sizeof(uv_fs_t));
    req->data = me;
    if (0 != (r = uv_fs_readdir(loop, req, directory, 0, __readdir_cb)))
    {
        printf("ERROR\n");
    }

    return (void*)me;
}

