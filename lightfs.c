#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#define IMG "fs.img"
#define TYPEDIR 1
#define TYPEFILE 2

char data[] = "Welcome to Lightfs!\n"
              "Features:\n"
              "- Change dir with : cd <dir>\n"
              "- Create folder with : mkdir <foldername>\n"
              "- Create file with : new <filename> <filedata>\n"
              "- Look at File with : cat <filename>\n"
              "- List folders and Files : ls\n";

int movement_parent = 0;
int old_parent = 0;

typedef struct DirBlock {
  char name[60];
  int offset;
  int isfree;
  int parent_offset;
} DirBlock;

typedef struct {
  char name[60];
  char data[4096];
  int parent_offset;
  int isfree;
  int offset;
} FileBlock;

struct Overwrite {
  int diroffset;
  int fileoffset;
  int overwritable;
};

DirBlock *dirs = NULL;
struct Overwrite write;

int overwrite(int mode) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    return -1;
  }
  int type;
  if (mode == 0) {
    DirBlock dir;
    while (fread(&type, sizeof(int), 1, img) == 1) {
      long pos = ftell(img) - sizeof(int);
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
    FileBlock file;
    while (fread(&type, sizeof(int), 1, img) == 1) {
      long pos = ftell(img) - sizeof(int);
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

void newdir(const char *dirname, int parent_offset) {
  FILE *img;
  DirBlock dir;
  strncpy(dir.name, dirname, 60);
  dir.name[sizeof(dir.name) - 1] = '\0';
  dir.isfree = 0;
  dir.parent_offset = parent_offset;
  int type = TYPEDIR;

  long write_pos;

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
  dir.offset = (int)ftell(img);
  fwrite(&dir, sizeof(DirBlock), 1, img);

  fclose(img);
}
void newfile(const char *filename, char *data, int parent_offset) {
  FILE *img;

  FileBlock file;
  strncpy(file.name, filename, 60);
  file.name[sizeof(file.name) - 1] = '\0';
  memset(file.data, 0, sizeof(file.data));
  file.data[sizeof(file.data) - 1] = '\0';
  strncpy(file.data, data, sizeof(file.data) - 1);
  file.parent_offset = parent_offset;
  file.isfree = 0;
  int type = TYPEFILE;
  long write_pos;

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
  file.offset = (int)ftell(img);
  fwrite(&file, sizeof(FileBlock), 1, img);
  fclose(img);
}

void list(int dir_offset) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    perror("file read errror");
    return;
  }

  DirBlock dir;
  FileBlock file;

  int type;
  while (fread(&type, sizeof(int), 1, img) == 1) {
    if (type == TYPEDIR) {
      fread(&dir, sizeof(DirBlock), 1, img);
      if (dir.isfree == 0 && dir.parent_offset == dir_offset) {
        printf("[DIR] %s\n", dir.name);
      }
    } else if (type == TYPEFILE) {
      fread(&file, sizeof(FileBlock), 1, img);
      if (file.isfree == 0 && file.parent_offset == dir_offset) {
        printf("[FILE] %s\n", file.name);
      }
    } else {
      printf("File/Folder didnt read.");
      break;
    }
  }

  fclose(img);
}

int doffset(const char *dirname) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    perror("foffset file open failed");
    return -1;
  }

  int type;

  DirBlock dir;
  while (fread(&type, sizeof(int), 1, img) == 1) {
    if (type == TYPEDIR) {
      fread(&dir, sizeof(DirBlock), 1, img);
      if (dir.isfree == 0 && strcmp(dir.name, dirname) == 0) {
        fclose(img);
        return dir.offset;
      }
    } else if (type == TYPEFILE) {
      fseek(img, sizeof(FileBlock), SEEK_CUR);
    } else {
      break;
    }
  }
  fclose(img);
  return 0;
}

void file_del(const char *filename, int parent_offset) {
  FILE *img = fopen(IMG, "r+b");
  if (img == NULL) {
    perror("file open error");
    return;
  }

  int type;
  FileBlock file;
  DirBlock dir;
  long pos;
  while (fread(&type, sizeof(int), 1, img) == 1) {
    pos = ftell(img);
    if (type == TYPEFILE) {
      fread(&file, sizeof(FileBlock), 1, img);
      if (file.parent_offset == parent_offset &&
          strcmp(file.name, filename) == 0) {
        file.isfree = 1;
        fseek(img, pos, SEEK_SET);
        fwrite(&file, sizeof(FileBlock), 1, img);
        fclose(img);
        return;
      }
    } else if (type == TYPEDIR) {
      fseek(img, sizeof(DirBlock), SEEK_CUR);
    } else {
      perror("type is not found");
      return;
    }
  }
  fclose(img);
  fprintf(stderr, "file not found\n");
}

void del_dir(const char *dirname, int parent_offset) {
  FILE *img = fopen(IMG, "r+b");
  if (img == NULL) {
    perror("delete dir failed open error");
    return;
  }

  int type;
  DirBlock dir;
  long pos;
  while (fread(&type, sizeof(int), 1, img) == 1) {
    pos = ftell(img);
    if (type == TYPEDIR) {
      fread(&dir, sizeof(DirBlock), 1, img);
      if (dir.parent_offset == parent_offset &&
          strcmp(dir.name, dirname) == 0) {
        dir.isfree = 1;
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

void lookatdata(const char *filename, int parent_offset, char *out,
                size_t size) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    perror("lookatdata file open error");
    return;
  }

  int type;
  FileBlock file;

  while (fread(&type, sizeof(int), 1, img) == 1) {
    if (type == TYPEFILE) {
      fread(&file, sizeof(FileBlock), 1, img);
      if (file.isfree == 0 && file.parent_offset == parent_offset &&
          strcmp(file.name, filename) == 0) {
        strncpy(out, file.data, size - 1);
        out[size - 1] = '\0';
        fclose(img);
        return;
      }
    } else if (type == TYPEDIR) {
      fseek(img, sizeof(DirBlock), SEEK_CUR);
    } else {
      return;
    }
  }

  fclose(img);
  return;
}

void cd(char *foldername) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    perror("cd open failed");
    return;
  }
  DirBlock dir;
  int type;
  while (fread(&type, sizeof(int), 1, img) == 1) {
    if (type == TYPEDIR) {
      fread(&dir, sizeof(DirBlock), 1, img);
      if (strcmp(foldername, "..") == 0) {
        if (dir.offset == movement_parent) {
          movement_parent = dir.parent_offset;
          fclose(img);
          return;
        }
      } else if (dir.isfree == 0 && dir.parent_offset == movement_parent &&
                 strcmp(dir.name, foldername) == 0) {
        old_parent = movement_parent;
        movement_parent = dir.offset;
        fclose(img);
        return;
      }
    } else if (type == TYPEFILE) {
      fseek(img, sizeof(FileBlock), SEEK_CUR);
    }
  }

  printf("Folder '%s' not found.\n", foldername);
  fclose(img);
}
void pwd(char *pwdout) {
  FILE *img = fopen(IMG, "rb");
  if (img == NULL) {
    perror("pwd open failed");
    return;
  }

  DirBlock dir;
  int type;
  char path[1024] = "";
  int curroffset = movement_parent;

  while (curroffset != 0) {
    fseek(img, 0, SEEK_SET);
    while (fread(&type, sizeof(type), 1, img) == 1) {
      if (type == TYPEDIR) {
        fread(&dir, sizeof(DirBlock), 1, img);
        if (dir.offset == curroffset) {
          char temp[1024];
          snprintf(temp, sizeof(temp), "/%s%s", dir.name, path);
          strcpy(path, temp);
          curroffset = dir.parent_offset;
          break;
        }
      } else if (type == TYPEFILE) {
        fseek(img, sizeof(FileBlock), SEEK_CUR);
      }
    }
  }

  strcpy(pwdout, path + 1);

  fclose(img);
}

int main() {
  remove(IMG);
  int loop = 1;
  newdir("quickstart", 0);
  newfile("quickstart.txt", data, doffset("quickstart"));

  while (loop) {
    char pwdout[256];
    char input[512];
    pwd(pwdout);
    printf("/%s -> ", pwdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
      continue;
    }

    input[strcspn(input, "\n")] = 0;

    char *command = strtok(input, " ");
    if (command == NULL) {
      continue;
    }

    if (strcmp(command, "ls") == 0) {
      list(movement_parent);
    } else if (strcmp(command, "exit") == 0) {
      loop = 0;
    } else if (strcmp(command, "cd") == 0) {
      char *folder = strtok(NULL, " ");
      if (folder != NULL) {
        cd(folder);
      } else {
        printf("undefined folder\n");
      }
    } else if (strcmp(command, "cat") == 0) {
      char *file = strtok(NULL, " ");
      if (file != NULL) {
        char out[4096];
        lookatdata(file, movement_parent, out, sizeof(out));
        printf("%s\n", out);
      } else {
        printf("undefined file\n");
      }
    } else if (strcmp(command, "mkdir") == 0) {
      char *name = strtok(NULL, " ");
      if (name != NULL) {
        newdir(name, movement_parent);
      } else {
        printf("undefined name\n");
      }
    } else if (strcmp(command, "new") == 0) {
      char *name = strtok(NULL, " ");
      char *data = strtok(NULL, " ");
      char buffer[4096] = "";
      while (data != NULL) {
        strcat(buffer, data);
        strcat(buffer, " ");
        data = strtok(NULL, " ");
      }
      newfile(name, buffer, movement_parent);
    } else if (strcmp(command, "rmdir") == 0) {
      char *name = strtok(NULL, " ");
      del_dir(name, movement_parent);
    } else if (strcmp(command, "rm") == 0) {
      char *name = strtok(NULL, " ");
      file_del(name, movement_parent);
    } else {
      printf("unrecognized command: %s\n", command);
    }
  }
  return 0;
}
