#include <complex.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lightfs.h"


#if defined(_WIN32)
    #include <io.h>
    #include <fcntl.h>
    #define TRUNCATE _chsize
#else
    #include <unistd.h>
    #define TRUNCATE ftruncate
#endif

int
file_truncate(FILE *fp, long new_size)
{
    int fd = fileno(fp);
    return TRUNCATE(fd, new_size);
}


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

void write_files(FileBlock file, FILE* img) {
	fwrite(&file.meta, sizeof(MetaBlock), 1, img);
	int data_start = (sizeof(MetaBlock) + sizeof(FileMetaBlock) + file.meta.name_size);
	file.block.data_start = data_start;
	fwrite(&file.block, sizeof(FileMetaBlock), 1, img);
	fwrite(file.name, 1, file.meta.name_size, img);
	fwrite(file.data, 1, file.block.data_size, img);
}

void write_dirs(DirBlock dir, FILE *img) {
	fwrite(&dir.meta, sizeof(MetaBlock), 1, img);
	fwrite(dir.name, 1, dir.meta.name_size, img);
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
doffset(const char* img_name, const char *dirname)
{
	FILE           *img = fopen(img_name, "rb");
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
				int offset = dir.meta.offset;
				free_dirs(&dir);
				return offset;
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
foffset(const char* img_name, const char *filename)
{
	FILE           *img = fopen(img_name, "rb");
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
				int offset = file.meta.offset;
				free_files(&file);
				return offset;
			}
			free_files(&file);
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
fixoffset(FILE *img, int removed_start, int removed_size)
{
    fseek(img, 0, SEEK_SET);

    while (1) {
        long entry_start = ftell(img);
        int type;

        if (fread(&type, sizeof(int), 1, img) != 1)
            break;

        if (type == TYPEFILE) {
            FileBlock file;
            read_files(&file, img);


            int removed_end = removed_start + removed_size;

            if (file.meta.offset >= removed_end)
                file.meta.offset -= removed_size;

            if (file.meta.parent_offset >= removed_end)
                file.meta.parent_offset -= removed_size;


            fseek(img, entry_start + sizeof(int), SEEK_SET);
            write_files(file, img);

            free_files(&file);
        }
        else if (type == TYPEDIR) {
            DirBlock dir;
            read_dirs(&dir, img);


            int removed_end = removed_start + removed_size;

            if (dir.meta.offset >= removed_end) {
                dir.meta.offset -= removed_size;
            }
            else if (dir.meta.offset >= removed_start) {
                dir.meta.offset = -1;
            }

            if (dir.meta.parent_offset >= removed_end) {
                dir.meta.parent_offset -= removed_size;
            }
            else if (dir.meta.parent_offset >= removed_start) {
                dir.meta.parent_offset = -1;
            }
            fseek(img, entry_start + sizeof(int), SEEK_SET);
            write_dirs(dir, img);

            free_dirs(&dir);
        }
        else {
            /* fs corruption */
            break;
        }
    }
}


void
shiftIT(const char* img_name, Shifting shift) {
    FILE *img = fopen(img_name, "r+b");
    if (img == NULL) {
        perror("shiftit file open failed");
        return;
    }

    DirBlock dir;
    FileBlock file;

    fseek(img, 0, SEEK_END);
    int filesize = (int) ftell(img);

    if (shift.type == TYPEFILE) {
        int offsetoffile = foffset(img_name,shift.name);
        fseek(img, offsetoffile, SEEK_SET);
        int shift_file_start = (int) ftell(img) - sizeof(int); // int -> type
        read_files(&file, img);
        int all_size_of_data = sizeof(int) + sizeof(file.meta) + sizeof(file.block) + file.meta.name_size + file.block.data_size;

        int end_offset = shift_file_start + all_size_of_data;

        fseek(img, end_offset, SEEK_SET);
        int buffer_size = filesize - end_offset;
        if (buffer_size < 0) {
            fclose(img);
            return;
        }
        char* buffer = malloc(buffer_size);
        if (!buffer) {
            fclose(img);
            return;
        }

        fread(buffer, 1, buffer_size, img);

        fseek(img, shift_file_start, SEEK_SET);
        fwrite(buffer, 1, buffer_size, img);

        fflush(img);

        fixoffset(img, shift_file_start, all_size_of_data);
        file_truncate(img, filesize - all_size_of_data);
        free(buffer);
        free_files(&file);
    }
    else {
        int offsetoffolder = doffset(img_name, shift.name);
        fseek(img, offsetoffolder, SEEK_SET);
        int shift_folder_start = (int) ftell(img) - sizeof(int); // int -> type
        read_dirs(&dir, img);
        int all_size_of_data = sizeof(int) + sizeof(dir.meta) + dir.meta.name_size;

        int end_offset = shift_folder_start + all_size_of_data;

        fseek(img, end_offset, SEEK_SET);
        int buffer_size = filesize - end_offset;
        if (buffer_size < 0) {
            fclose(img);
            return;
        }
        char* buffer = malloc(buffer_size);
        if (!buffer) {
            fclose(img);
            return;
        }

        fread(buffer, 1, buffer_size, img);

        fseek(img, shift_folder_start, SEEK_SET);
        fwrite(buffer, 1, buffer_size, img);

        fflush(img);
        fixoffset(img, shift_folder_start, all_size_of_data);
        file_truncate(img, filesize - all_size_of_data);
        free(buffer);
        free_dirs(&dir);
    }

}

void rmdir_recursive(const char* img_name, char* name) {
    int offset = doffset(img_name, name);

    FILE *img = fopen(img_name, "rb");
    if (!img) return;

    int type;
    DirBlock dir;
    FileBlock file;

    fseek(img, 0, SEEK_SET);
    while (fread(&type, sizeof(int), 1, img) == 1) {
        if (type == TYPEFILE) {
            read_files(&file, img);

            if (file.meta.isfree == 0 &&
                file.meta.parent_offset == offset) {

                Shifting shift;
                shift.name = file.name;
                shift.type = TYPEFILE;

                shiftIT(img_name, shift);

                img = fopen(img_name, "rb");
                fseek(img, 0, SEEK_SET);
                free_files(&file);
                continue;
            }
            free_files(&file);
        } else if (type == TYPEDIR) {
            seek_dir(img);
        }
    }

    fseek(img, 0, SEEK_SET);
    while (fread(&type, sizeof(int), 1, img) == 1) {
        if (type == TYPEDIR) {
            read_dirs(&dir, img);

            if (dir.meta.isfree == 0 &&
                dir.meta.parent_offset == offset) {

                int child_offset = dir.meta.offset;
                fclose(img);

                rmdir_recursive(img_name, dir.name);

                img = fopen(img_name, "rb");
                fseek(img, 0, SEEK_SET);
                free_dirs(&dir);
                continue;
            }
            free_dirs(&dir);
        } else if (type == TYPEFILE) {
            seek_files(img);
        }
    }

    Shifting s;
    s.type = TYPEDIR;
    s.name = name;
    shiftIT(img_name, s);
    fclose(img);
}


void
newdir(const char* img_name, const char *dirname, int parent_offset)
{
    if (doffset(img_name,dirname) != 0) {

        return;
    };
	FILE           *img;
	DirBlock 	dir;
	dir.meta.name_size = strlen(dirname) + 1;
	dir.name = malloc(dir.meta.name_size);
	strncpy(dir.name, dirname, dir.meta.name_size);
	dir.meta.isfree = 0;
	dir.meta.parent_offset = parent_offset;
	int 		type = TYPEDIR;

	long 		write_pos;

		img = fopen(img_name, "ab");
		write_pos = ftell(img);

	if (img == NULL) {
		perror("file open error");
		return;
	}
	fseek(img, write_pos, SEEK_SET);
	fwrite(&type, sizeof(int), 1, img);
	dir.meta.offset = (int) ftell(img);
	write_dirs(dir, img);
	free(dir.name);
	fclose(img);
}

void
newfile(const char* img_name, const char *filename, char *data, int parent_offset)
{
    if (foffset(img_name, filename) != 0) {
        return;
    }
	FILE           *img;
	FileBlock 	file;
	file.meta.name_size = strlen(filename) + 1;
	file.name = malloc(file.meta.name_size);
	strncpy(file.name, filename, file.meta.name_size);
	file.block.data_size = strlen(data) + 1;
	file.data = malloc(file.block.data_size);
	strncpy(file.data, data, file.block.data_size);
	file.meta.parent_offset = parent_offset;
	file.meta.isfree = 0;
	int 		type = TYPEFILE;
	long 		write_pos;


		img = fopen(img_name, "ab");
		write_pos = ftell(img);

	if (img == NULL) {
		perror("file open error");
		return;
	}
	fseek(img, write_pos, SEEK_SET);
	fwrite(&type, sizeof(int), 1, img);
	file.meta.offset = (int) ftell(img);
	write_files(file, img);
	free(file.name);
	free(file.data);
	fclose(img);
}

void
list(const char* img_name, int dir_offset)
{
	FILE           *img = fopen(img_name, "rb");
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
			free_dirs(&dir);
		} else if (type == TYPEFILE) {
		    read_files(&file, img);
			if (file.meta.isfree == 0 && file.meta.parent_offset == dir_offset) {
				printf("[FILE] %s\n", file.name);
			}
			free_files(&file);
		}
	}

	fclose(img);
}


void
lookatdata(const char* img_name, const char *filename, int parent_offset, char *out,
	   size_t size)
{
	FILE           *img = fopen(img_name, "rb");
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
				free_files(&file);
				fclose(img);
				return;
			}
			free_files(&file);
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
cd(const char* img_name, char *foldername)
{
	FILE           *img = fopen(img_name, "rb");
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
				free_dirs(&dir);
			} else if (dir.meta.isfree == 0 && dir.meta.parent_offset == movement_parent &&
				   strcmp(dir.name, foldername) == 0) {
				old_parent = movement_parent;
				movement_parent = dir.meta.offset;
				fclose(img);
				free_dirs(&dir);
				return;
			}
			free_dirs(&dir);
		} else if (type == TYPEFILE) {
		    seek_files(img);
		}
	}
	printf("Folder '%s' not found.\n", foldername);
	fclose(img);
}

void
pwd(const char* img_name, char *pwdout)
{
	FILE           *img = fopen(img_name, "rb");
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
					free_dirs(&dir);
					break;
				}
				free_dirs(&dir);
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
	newdir(IMG, "quickstart", 0);
	newfile(IMG, "quickstart.txt", data, doffset(IMG,"quickstart"));

	while (loop) {
		char 		pwdout   [256];
		char 		input    [512];
		pwd(IMG,pwdout);
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
			list(IMG, movement_parent);
		} else if (strcmp(command, "exit") == 0) {
			loop = 0;
		} else if (strcmp(command, "cd") == 0) {
			char           *folder = strtok(NULL, " ");
			if (folder != NULL) {
				cd(IMG, folder);
			} else {
				printf("undefined folder\n");
			}
		} else if (strcmp(command, "cat") == 0) {
			char           *file = strtok(NULL, " ");
			if (file != NULL) {
				char 		out      [4096];
				lookatdata(IMG, file, movement_parent, out, sizeof(out));
				printf("%s\n", out);
			} else {
				printf("undefined file\n");
			}
		} else if (strcmp(command, "mkdir") == 0) {
			char           *name = strtok(NULL, " ");
			if (name != NULL) {
				newdir(IMG, name, movement_parent);
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
			newfile(IMG, name, buffer, movement_parent);
		} else if (strcmp(command, "rmdir") == 0) {
			char           *name = strtok(NULL, " ");
			rmdir_recursive(IMG, name);
		} else if (strcmp(command, "rm") == 0) {
			char           *name = strtok(NULL, " ");
			Shifting shift;
			shift.name = name;
			shift.type = TYPEFILE;
			shiftIT(IMG, shift);
		} else {
			printf("unrecognized command: %s\n", command);
		}
	}
	return 0;
}
