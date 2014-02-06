
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdio.h>

/* for abs() */
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <uv.h>

/* for time() */
#include <time.h>

/* for log(), pow() */
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for priority queue */
#include "heap.h"

/* for file map */
#include "linked_list_hashmap.h"

#include "fff.h"

typedef struct file_s file_t;

typedef struct {
    /* monitored directory */
    char* path;

    uv_loop_t *loop;

    hashmap_t *files;

    /* scanning priority queue */
    heap_t *squ;

    /* we populate this when migrating away from squ */
    heap_t *squ_aux;

    filewatcher_cbs_t cb;
    void* cb_ctx;
} filewatcher_private_t;

struct file_s {

    /* currently this is the file path */
    char* name;

    /* size in bytes */
    unsigned int size;

    /* modification time */
    unsigned int mtime;

    void* udata;

    int is_dir;

    int nchildren;

    /* number of event points */
    int pts;

    /* how often we scan this directory */
    int scan_priority;
};

#if WIN32
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
#endif

static void __readdir_cb(uv_fs_t* req);
static void __stat_cb(uv_fs_t* req);
void __add_file(filewatcher_private_t* me, file_t* f);

#if 0
void __print_file(file_t* f, int depth)
{
    printf("%s %dB %llu\n", f->name, f->size, f->mtime);
}
#endif

#if 0
static void __log(void *udata, void *src, const char *buf, ...)
{
    //printf("%s\n", buf);
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
    f->is_dir = (req->statbuf.st_mode & S_IFDIR) ? 1 : 0;

    if (!hashmap_get(me->files, f->name))
    {
        __add_file(me, f);

        /* put directory in queue */
        if (f->is_dir)
            heap_offer(me->squ_aux, f);
    }

    uv_fs_req_cleanup(req);
}

/**
 * run a stat() on each file in directory */
static void __readdir_cb(uv_fs_t* req)
{
    filewatcher_private_t* me;
    char* fname; /* file name */
    file_t* dir;
    int i;

    dir = req->data;
    me = dir->udata;

    if (me->cb.file_scanned)
        me->cb.file_scanned(me->cb_ctx, dir->name);

    for (i=0, fname = req->ptr; i<req->result; i++)
    {
        file_t *f;
        char path[512];

        /* never seen this file before */
        snprintf(path,512,"%s/%s", req->path, fname);
        if (!(f = hashmap_get(me->files, path)))
        {
            f = calloc(1,sizeof(file_t));
            f->udata = me;
            asprintf(&f->name, "%s/%s", req->path, fname);
        }


        /* stat() the file */
        // TODO replace with mempool/arena
        int r;
        uv_fs_t *stat_req = malloc(sizeof(uv_fs_t));
        stat_req->data = f;
        if (0 != (r = uv_fs_stat(me->loop, stat_req, f->name, __stat_cb)))
        {
            printf("ERROR\n");
        }
        
        fname += strlen(fname) + 1;
    }

    //__log(me->logger, "scanned, dir:%s, children:%d", dir->name, i);

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

/**
 * Process events that are dependent on time passing
 * @param msec_elapsed Time in milliseconds since the last call
 * @return 1 on success; otherwise 0 */
int fff_periodic(filewatcher_t* me_, int msec_elapsed)
{
    filewatcher_private_t* me = (void*)me_;
    file_t* f;

    if ((f = heap_poll(me->squ)))
    {
        uv_fs_t *req;
        int r;

        assert(1 == f->is_dir);

        req = calloc(1,sizeof(uv_fs_t));
        req->data = f;
        if (0 != (r = uv_fs_readdir(me->loop, req, f->name, 0, __readdir_cb)))
        {
            printf("ERROR\n");
        }

        f->scan_priority -= 1;

        /* move to aux queue if it's time */
        if (f->scan_priority <= 0 || 1 == rand() % 2)
        {
            f->scan_priority =
                1 + abs(log(pow(f->pts,2) * ((double)(rand() % 100))/100));
            heap_offer(me->squ_aux,f);
        }
    }
    else /* queue is empty, switch to other queue */
    {
        heap_t* swp;
        swp = me->squ;
        me->squ = me->squ_aux;
        me->squ_aux = swp;
        //__log(me->logger, "swapped queue");
    }

    return 1;
}

static int __cmp_file_scan_priority(
    const void *i1,
    const void *i2,
    const void *ckr
)
{
    const file_t *f1 = i1;
    const file_t *f2 = i2;
    return f2->scan_priority - f1->scan_priority;
}

filewatcher_t *fff_new(
        char* directory,
        uv_loop_t* loop,
        filewatcher_cbs_t *cbs,
        void* cb_ctx)
{
    filewatcher_private_t* me;

    srand(time(NULL));

    me = calloc(1,sizeof(filewatcher_private_t));
    me->squ = heap_new(__cmp_file_scan_priority, NULL);
    me->squ_aux = heap_new(__cmp_file_scan_priority, NULL);
    me->files = hashmap_new(__hash_file, __cmp_file, 11);
    me->loop = loop;
    memcpy(&me->cb, cbs, sizeof(filewatcher_cbs_t));

    /* put first directory in queue */
    file_t *f = calloc(1,sizeof(file_t));
    asprintf(&f->name, "%s", directory);
    f->udata = me;
    f->is_dir = 1;
    heap_offer(me->squ, f);

    return (void*)me;
}

