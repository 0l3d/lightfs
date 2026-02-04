# Lightfs

Lightfs is lightweight and simple filesystem simulator and single header library.

## Features
- Change dir with : cd <dir>
- Create folder with : mkdir <foldername>
- Create file with : new <filename> <filedata>
- Look at File with : cat <filename>
- List folders and Files : ls
- Remove Files: rm <filename>
- Remove Folders: rmdir <foldername>

## Usage and Files

```
git clone https://github.com/0l3d/lightfs.git
cd lightfs/
gcc lightfs.c -o lightfs
./lightfs
-> cd quickstart
-> cat quickstart.txt
```

The entire file system is stored in the `fs.img` file.

# License

This project is licensed under the **GPL-3.0 License**.

# Author

Created By **0l3d**.
