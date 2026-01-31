#include <complex.h>
#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "lightfs.h"

char 		data     [] = "Welcome to Lightfs!\n"
"Features:\n"
"- Change dir with : cd <dir>\n"
"- Create folder with : mkdir <foldername>\n"
"- Create file with : new <filename> <filedata>\n"
"- Look at File with : cat <filename>\n"
"- List folders and Files : ls\n";

int 		movement_parent = 0;
int 		old_parent = 0;

DirBlock       *dirs = NULL;
struct Overwrite write;

int
overwrite(int mode) {
    return -1;
}
/*
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		return -1;
	}
	int 		type;
	if (mode == 0) {
		DirBlock 	dir;
		while (fread(&type, sizeof(int), 1, img) == 1) {
			long 		pos = ftell(img) - sizeof(int);
			if (type == TYPEDIR) {
				fread(&dir, sizeof(DirBlock), 1, img);
				if (dir.isfree == 1) {
					write.overwritable = 1;
					write.diroffset = pos;
					fclose(img);
					return 0;
				}
			} else {
				if (type == TYPEFILE)
					fseek(img, sizeof(FileBlock), SEEK_CUR);
				else
					break;
			}
		}
	} else {
		FileBlock 	file;
		while (fread(&type, sizeof(int), 1, img) == 1) {
			long 		pos = ftell(img) - sizeof(int);
			if (type == TYPEFILE) {
				fread(&file, sizeof(FileBlock), 1, img);
				if (file.isfree == 1) {
					write.overwritable = 1;
					write.fileoffset = pos;
					fclose(img);
					return 1;
				}
			} else {
				if (type == TYPEDIR)
					fseek(img, sizeof(DirBlock), SEEK_CUR);
				else
					break;
			}
		}
	}
	fclose(img);
	return -1;
}
*/

void
read_dirs(DirBlock * dir, FILE * img)
{
	fread(&dir->meta, sizeof(MetaBlock), 1, img);
	dir->name = malloc(dir->meta.name_size);
	fread(dir->name, 1, dir->meta.name_size, img);
}

void free_dirs(DirBlock *dir) {
    free(dir->name);
}

void seek_dir(FILE *img) {
	DirBlock dir;
	fread(&dir.meta, sizeof(MetaBlock), 1, img);
	fseek(img, dir.meta.name_size, SEEK_CUR);
}

void read_files(FileBlock *file, FILE* img) {
    fread(&file->meta, sizeof(MetaBlock), 1, img);
    fread(&file->block, sizeof(FileMetaBlock), 1, img);
    file->name = malloc(file->meta.name_size);
    fread(file->name, 1, file->meta.name_size, img);
    file->data = malloc(file->block.data_size);
    fread(file->data, 1, file->block.data_size, img);
}

void free_files(FileBlock *file) {
    free(file->name);
    free(file->data);
}

void seek_files(FILE* img) {
    FileBlock file;
    fread(&file.meta, sizeof(MetaBlock), 1, img);
    fread(&file.block, sizeof(FileMetaBlock), 1, img);
    fseek(img, file.meta.name_size, SEEK_CUR);
    fseek(img, file.block.data_size, SEEK_CUR);
}

int
doffset(const char *dirname)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("doffset file open failed");
		return -1;
	}
	int 		type;

	DirBlock 	dir;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		if (type == TYPEDIR) {
            read_dirs(&dir, img);
		    if (dir.meta.isfree == 0 && strcmp(dir.name, dirname) == 0) {
				fclose(img);
				return dir.meta.offset;
			}
		} else if (type == TYPEFILE) {
		    seek_files(img);
		} else {
			break;
		}
	}
	fclose(img);
	return 0;
}

int
foffset(const char *filename)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("foffset file open failed");
		return -1;
	}
	int 		type;

	FileBlock   file;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		if (type == TYPEFILE) {
            read_files(&file, img);
		    if (file.meta.isfree == 0 && strcmp(file.name, filename) == 0) {
				fclose(img);
				return file.meta.offset;
			}
		} else if (type == TYPEDIR) {
		    seek_dir(img);
		} else {
			break;
		}
	}
	fclose(img);
	return 0;
}

void
shiftIT(Shifting shift) {
    FILE *img = fopen(IMG, "r+b");
    if (img == NULL) {
        perror("shiftit file open failed");
        return;
    }

    DirBlock dir;
    FileBlock file;

    fseek(img, 0, SEEK_END);
    int filesize = (int) ftell(img);

    if (shift.type == TYPEFILE) {
        int offsetoffile = foffset(shift.name);
        fseek(img, offsetoffile, SEEK_SET);
        int file_start = (int) ftell(img) - sizeof(int); // int -> type
        read_files(&file, img);
        int all_size_of_data = sizeof(file.meta) + sizeof(file.block) + file.meta.name_size + file.block.data_size;
        // int size_fromend = (all_size_of_data - file_start) -

    }
    else {}

}



void
newdir(const char *dirname, int parent_offset)
{
    if (doffset(dirname) != 0) {
        return;
    };
	FILE           *img;
	DirBlock 	dir;
	dir.meta.name_size = strlen(dirname) + 1;
	dir.name = malloc(dir.meta.name_size);
	strncpy(dir.name, dirname, dir.meta.name_size);
	dir.name[dir.meta.name_size] = '\0';
	dir.meta.isfree = 0;
	dir.meta.parent_offset = parent_offset;
	int 		type = TYPEDIR;

	long 		write_pos;

	if (overwrite(0) != -1) {
		img = fopen(IMG, "r+b");
		write_pos = write.diroffset;
	} else {
		img = fopen(IMG, "ab");
		write_pos = ftell(img);
	}

	if (img == NULL) {
		perror("file open error");
		return;
	}
	fseek(img, write_pos, SEEK_SET);
	fwrite(&type, sizeof(int), 1, img);
	dir.meta.offset = (int) ftell(img);
	fwrite(&dir.meta, sizeof(MetaBlock), 1, img);
	fwrite(dir.name, 1, dir.meta.name_size, img);
	fclose(img);
}

void
newfile(const char *filename, char *data, int parent_offset)
{
    if (foffset(filename) != 0) {
        return;
    }
	FILE           *img;
	FileBlock 	file;
	file.meta.name_size = strlen(filename);
	file.name = malloc(file.meta.name_size + 1);
	strncpy(file.name, filename, file.meta.name_size);
	file.name[file.meta.name_size] = '\0';
	file.block.data_size = strlen(data);
	file.data = malloc(file.block.data_size + 1);
	strncpy(file.data, data, file.block.data_size);
	file.data[file.block.data_size] = '\0';
	file.meta.parent_offset = parent_offset;
	file.meta.isfree = 0;
	int 		type = TYPEFILE;
	long 		write_pos;

	if (overwrite(1) != -1) {
		img = fopen(IMG, "r+b");
		write_pos = write.fileoffset;
	} else {
		img = fopen(IMG, "ab");
		write_pos = ftell(img);
	}
	if (img == NULL) {
		perror("file open error");
		return;
	}
	fseek(img, write_pos, SEEK_SET);
	fwrite(&type, sizeof(int), 1, img);
	file.meta.offset = (int) ftell(img);
	fwrite(&file.meta, sizeof(MetaBlock), 1, img);
	int data_start = (sizeof(MetaBlock) + sizeof(FileMetaBlock) + file.meta.name_size);
	file.block.data_start = data_start;
	fwrite(&file.block, sizeof(FileMetaBlock), 1, img);
	fwrite(file.name, 1, file.meta.name_size, img);
	fwrite(file.data, 1, file.block.data_size, img);
	fclose(img);
}

void
list(int dir_offset)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("file read errror");
		return;
	}
	DirBlock 	dir;
	FileBlock 	file;

	int 		type;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		if (type == TYPEDIR) {
			read_dirs(&dir, img);
			if (dir.meta.isfree == 0 && dir.meta.parent_offset == dir_offset) {
				printf("[DIR] %s\n", dir.name);
			}
		} else if (type == TYPEFILE) {
		    read_files(&file, img);
			if (file.meta.isfree == 0 && file.meta.parent_offset == dir_offset) {
				printf("[FILE] %s\n", file.name);
			}
		} else {
			printf("File/Folder didnt read.");
			break;
		}
	}

	fclose(img);
}



void
file_del(const char *filename, int parent_offset) {}
/*
{
	FILE           *img = fopen(IMG, "r+b");
	if (img == NULL) {
		perror("file open error");
		return;
	}
	int 		type;
	FileBlock 	file;
	DirBlock 	dir;
	long 		pos;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		pos = ftell(img);
		if (type == TYPEFILE) {
			fread(&file, sizeof(FileBlock), 1, img);
			if (file.meta.parent_offset == parent_offset &&
			    strcmp(file.name, filename) == 0) {
				file.meta.isfree = 1;
				fseek(img, pos, SEEK_SET);
				fwrite(&file, sizeof(FileBlock), 1, img);
				fclose(img);
				return;
			}
		} else if (type == TYPEDIR) {
			seek_dir(img);
		} else {
			perror("type is not found");
			return;
		}
	}
	fclose(img);
	fprintf(stderr, "file not found\n");
}
*/

void
del_dir(const char *dirname, int parent_offset) {}
/*
{
	FILE           *img = fopen(IMG, "r+b");
	if (img == NULL) {
		perror("delete dir failed open error");
		return;
	}
	int 		type;
	DirBlock 	dir;
	long 		pos;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		pos = ftell(img);
		if (type == TYPEDIR) {
			fread(&dir, sizeof(DirBlock), 1, img);
			if (dir.meta.parent_offset == parent_offset &&
			    strcmp(dir.name, dirname) == 0) {
				dir.meta.isfree = 1;
				fseek(img, pos, SEEK_SET);
				fwrite(&dir, sizeof(DirBlock), 1, img);
				fclose(img);
				return;
			}
		} else if (type == TYPEFILE) {
			fseek(img, sizeof(FileBlock), SEEK_CUR);
		} else {
			break;
		}
	}
}
*/

void
lookatdata(const char *filename, int parent_offset, char *out,
	   size_t size)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("lookatdata file open error");
		return;
	}
	int 		type;
	FileBlock 	file;

	while (fread(&type, sizeof(int), 1, img) == 1) {
		if (type == TYPEFILE) {
			read_files(&file, img);
			if (file.meta.isfree == 0 && file.meta.parent_offset == parent_offset &&
			    strcmp(file.name, filename) == 0) {
				strncpy(out, file.data, size - 1);
				out[size - 1] = '\0';
				fclose(img);
				return;
			}
		} else if (type == TYPEDIR) {
			DirBlock dir;
			seek_dir(img);
		} else {
			return;
		}
	}

	fclose(img);
	return;
}

void
cd(char *foldername)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("cd open failed");
		return;
	}

	DirBlock 	dir;
	int 		type;
	while (fread(&type, sizeof(int), 1, img) == 1) {
		if (type == TYPEDIR) {
		    read_dirs(&dir, img);
			if (strcmp(foldername, "..") == 0) {
				if (dir.meta.offset == movement_parent) {
					movement_parent = dir.meta.parent_offset;
					fclose(img);
					free_dirs(&dir);
					return;
				}
			} else if (dir.meta.isfree == 0 && dir.meta.parent_offset == movement_parent &&
				   strcmp(dir.name, foldername) == 0) {
				old_parent = movement_parent;
				movement_parent = dir.meta.offset;
				fclose(img);
				free_dirs(&dir);
				return;
			}
		} else if (type == TYPEFILE) {
		    seek_files(img);
		}
	}
	printf("Folder '%s' not found.\n", foldername);
	fclose(img);
}

void
pwd(char *pwdout)
{
	FILE           *img = fopen(IMG, "rb");
	if (img == NULL) {
		perror("pwd open failed");
		return;
	}
	DirBlock 	dir;
	int 		type;
	char 		path     [1024] = "";
	int 		curroffset = movement_parent;

	while (curroffset != 0) {
		fseek(img, 0, SEEK_SET);
		while (fread(&type, sizeof(type), 1, img) == 1) {
			if (type == TYPEDIR) {
				read_dirs(&dir, img);
				if (dir.meta.offset == curroffset) {
					char 		temp     [1024];
					snprintf(temp, sizeof(temp), "/%s%s", dir.name, path);
					strcpy(path, temp);
					curroffset = dir.meta.parent_offset;
					break;
				}
			} else if (type == TYPEFILE) {
				seek_files(img);
			}
		}
	}

	strcpy(pwdout, path + 1);

	fclose(img);
}

int
main()
{
	int 		loop = 1;
	newdir("quickstart", 0);
	newfile("quickstart.txt", data, doffset("quickstart"));

	Shifting shift;
	shift.type = TYPEFILE;
	shift.name = "quickstart.txt";
	shiftIT(shift);

	while (loop) {
		char 		pwdout   [256];
		char 		input    [512];
		pwd(pwdout);
		printf("/%s -> ", pwdout);

		if (fgets(input, sizeof(input), stdin) == NULL) {
			continue;
		}
		input[strcspn(input, "\n")] = 0;

		char           *command = strtok(input, " ");
		if (command == NULL) {
			continue;
		}
		if (strcmp(command, "ls") == 0) {
			list(movement_parent);
		} else if (strcmp(command, "exit") == 0) {
			loop = 0;
		} else if (strcmp(command, "cd") == 0) {
			char           *folder = strtok(NULL, " ");
			if (folder != NULL) {
				cd(folder);
			} else {
				printf("undefined folder\n");
			}
		} else if (strcmp(command, "cat") == 0) {
			char           *file = strtok(NULL, " ");
			if (file != NULL) {
				char 		out      [4096];
				lookatdata(file, movement_parent, out, sizeof(out));
				printf("%s\n", out);
			} else {
				printf("undefined file\n");
			}
		} else if (strcmp(command, "mkdir") == 0) {
			char           *name = strtok(NULL, " ");
			if (name != NULL) {
				newdir(name, movement_parent);
			} else {
				printf("undefined name\n");
			}
		} else if (strcmp(command, "new") == 0) {
			char           *name = strtok(NULL, " ");
			char           *data = strtok(NULL, " ");
			char 		buffer   [4096] = "";
			while (data != NULL) {
				strcat(buffer, data);
				strcat(buffer, " ");
				data = strtok(NULL, " ");
			}
			newfile(name, buffer, movement_parent);
		} else if (strcmp(command, "rmdir") == 0) {
			char           *name = strtok(NULL, " ");
			del_dir(name, movement_parent);
		} else if (strcmp(command, "rm") == 0) {
			char           *name = strtok(NULL, " ");
			file_del(name, movement_parent);
		} else {
			printf("unrecognized command: %s\n", command);
		}
	}
	return 0;
}
