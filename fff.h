

typedef void* filewatcher_t;

typedef struct {
    int (*file_added)(
            void* udata,
            char* name,
            int is_dir,
            unsigned int size,
            unsigned long mtime);
    int (*file_removed)(void* udata, char* name);
    int (*file_changed)(void* udata, char* name, int new_size, unsigned long mtime);
    int (*file_moved)(void* udata, char* name, char* new_name, unsigned long mtime);
} filewatcher_cbs_t;

/**
 * @param path The directory we're going to follower
 * @param loop libuv loop
 * @param cb_ctx context that all callbacks get called with
 * @return newly initialised file watcher */
filewatcher_t *fff_new(
        char* directory,
        uv_loop_t* loop,
        filewatcher_cbs_t *cbs,
        void* cb_ctx);

