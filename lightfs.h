
#ifndef DEFINE_LIGHTFS
#define DEFINE_LIGHTFS

#include <stdio.h>

#define TYPEDIR 1
#define TYPEFILE 2

typedef struct {
    FILE *img;
    int movement_parent;
    int old_parent;
} LightFS;

typedef struct {
    int name_size;
    int offset;
    int isfree;
    int parent_offset;
} MetaBlock;

typedef struct {
    char *name;
    int type;
} Shifting;

typedef struct {
    MetaBlock meta;
    char *name;
} DirBlock;

typedef struct {
    int data_size;
    int data_start;
} FileMetaBlock;

typedef struct {
    MetaBlock meta;
    char *name;
    FileMetaBlock block;
    char *data;
} FileBlock;

int  lfs_doffset(LightFS *fs, const char *name);
int  lfs_foffset(LightFS *fs, const char *name);

void lfs_newdir(LightFS *fs, const char *name);
void lfs_newfile(LightFS *fs, const char *name, char *data);

void lfs_list(LightFS *fs);
void lfs_cd(LightFS *fs, char *name);
void lfs_pwd(LightFS *fs, char *out);

void lfs_cat(LightFS *fs, const char *name, char *out, size_t size);

void lfs_rm(LightFS *fs, const char *name);
void lfs_rmdir_recursive(LightFS *fs, const char *name);

#endif
