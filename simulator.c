#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lightfs.h"

#define IMG "fs.img"

char 		data     [] = "Welcome to Lightfs!\n"
"Features:\n"
"- Change dir with : cd <dir>\n"
"- Create folder with : mkdir <foldername>\n"
"- Create file with : new <filename> <filedata>\n"
"- Look at File with : cat <filename>\n"
"- List folders and Files : ls\n"
"- Remove Files: rm <filename>\n"
"- Remove Folders: rmdir <foldername>\n";

int
main()
{
	LightFS 	fs;
	fs.img = fopen(IMG, "r+b");
	if (!fs.img) {
		fs.img = fopen(IMG, "w+b");
		if (!fs.img) {
			perror("Failed to open LightFS image");
			return 1;
		}
	}
	fs.movement_parent = 0;
	fs.old_parent = 0;

	lfs_newdir(&fs, "quickstart", 0);
	lfs_newfile(&fs, "quickstart.txt", data, lfs_doffset(&fs, "quickstart", 0));

	int 		loop = 1;
	while (loop) {
		char 		pwdout   [256];
		lfs_pwd(&fs, pwdout);
		printf("/%s -> ", pwdout);

		char 		input    [512];
		if (fgets(input, sizeof(input), stdin) == NULL) {
			continue;
		}
		input[strcspn(input, "\n")] = 0;

		char           *command = strtok(input, " ");
		if (!command)
			continue;

		if (strcmp(command, "ls") == 0) {
			ListFF 		f;
			lfs_list(&fs, &f);
			for (int i = 0; i < f.entrycount; i++) {
				if (f.entry[i]->type == TYPEDIR) {
					printf("[DIR] %s\n", f.entry[i]->name);
				} else {
					printf("[FILE] %s\n", f.entry[i]->name);
				}
			}
			lfs_free_list(&f);
		} else if (strcmp(command, "exit") == 0) {
			loop = 0;
		} else if (strcmp(command, "cd") == 0) {
			char           *folder = strtok(NULL, " ");
			if (strchr(folder, '/')) {
				lfs_go_path(&fs, folder);
			} else if (folder) {
				lfs_cd(&fs, folder);
			} else {
				printf("undefined folder\n");
			}
		} else if (strcmp(command, "cat") == 0) {
			char           *file = strtok(NULL, " ");
			if (file) {
				char           *out;
				lfs_cat(&fs, file, fs.movement_parent, &out);
				printf("%s\n", out);
				free(out);
			} else {
				printf("undefined file\n");
			}
		} else if (strcmp(command, "mkdir") == 0) {
			char           *name = strtok(NULL, " ");
			if (name) {
				lfs_newdir(&fs, name, fs.movement_parent);
			} else {
				printf("undefined name\n");
			}
		} else if (strcmp(command, "new") == 0) {
			char           *name = strtok(NULL, " ");
			char           *datapart = strtok(NULL, " ");
			if (name && datapart) {
				char 		buffer   [4096] = "";
				while (datapart) {
					strcat(buffer, datapart);
					strcat(buffer, " ");
					datapart = strtok(NULL, " ");
				}
				lfs_newfile(&fs, name, buffer, fs.movement_parent);
			} else {
				printf("undefined file or data\n");
			}
		} else if (strcmp(command, "rmdir") == 0) {
			char           *name = strtok(NULL, " ");
			if (name) {
				lfs_rmdir(&fs, name);
			} else {
				printf("undefined folder\n");
			}
		} else if (strcmp(command, "rm") == 0) {
			char           *name = strtok(NULL, " ");
			if (name) {
				lfs_rm(&fs, name);
			} else {
				printf("undefined file\n");
			}
		} else {
			printf("unrecognized command: %s\n", command);
		}
	}

	fclose(fs.img);
	return 0;
}
