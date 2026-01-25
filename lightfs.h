#ifndef DEFINE_LIGHTFS
#define DEFINE_LIGHTFS

#define IMG "file.img"
#define TYPEDIR 1
#define TYPEFILE 2

typedef struct {
	int 		name_size;
	int 		offset;
	int 		isfree;
	int 		parent_offset;
} MetaBlock;

typedef struct {
	MetaBlock 		meta;
	char           *name;
} DirBlock;

typedef struct {
	int data_size;
	int data_start;
} FileMetaBlock;

typedef struct {
	MetaBlock 		meta;
	char *name;
	FileMetaBlock block;
	char           *data;
} 		FileBlock;

struct Overwrite {
	int 		diroffset;
	int 		fileoffset;
	int 		overwritable;
};



#endif
