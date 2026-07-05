# Simple Ext4

This project implements a simplified EXT4 filesystem reader/writer over a disk image.
The program opens a `.img` file, loads key metadata (superblock, block group descriptors, bitmaps),
and provides an interactive shell for basic read/write operations.

## Objective

- Understand EXT4 internal structures (superblock, inodes, directory entries, bitmaps, extents).
- Implement shell commands for metadata and content inspection.
- Implement basic write operations (create/remove/rename).
- Work directly with on-disk byte parsing using fixed offsets (without relying on kernel structs).

## Implementation Scope and Restrictions

This project intentionally uses a limited scope to stay didactic and practical:

1. **Extents are mandatory**
- Initialization validates that the extents feature is enabled in the superblock (`s_feature_incompat` with bit `0x40`).
- It's common to not having this field in past EXT versions (like ext2/ext3), but extents are a core feature of EXT4.
- File and directory data read/write logic is implemented based on extents.

2. **No hash tree (htree) support**
- There is no directory indexing implementation via hash tree.
- Directory lookup uses a linear scan over directory entries read from blocks.

3. **Focused on basic commands**
- Implements core commands for study and demonstration of EXT4 organization.
- Does not cover all advanced EXT4 features (journal, very deep extent trees, full ACL support, etc.).

## Folder Structure

### `ext4/`
Main filesystem logic layer.

- `types.h`: structure definitions used by the project (superblock, inode, dir entry, extents, constants).
- `context.*`: image opening, superblock/BGDT/bitmap loading, validation, and global session state.
- `inode.*`: inode read/write, file data reading through extents, block allocation helper.
- `dir.*`: directory listing, name lookup, path resolution, entry insert/remove/rename.
- `bitmap.*`: inode/block bitmap test/allocation/free operations.

### `parsers/`
On-disk raw-byte parsing layer into in-memory structures.

- `superblock_parser.*`: extracts relevant superblock fields via fixed offsets.
- `block_group_descriptor_parser.*`: extracts essential block group descriptor fields.
- `inode_parser.*`: parses inodes from inode table bytes.
- `dir_entry_parser.*`: parses directory entries (`inode`, `rec_len`, `name_len`, `file_type`, `name`).

### `shell/`
User command-line interface.

- `shell.*`: REPL loop and command dispatch.
- `commands_read.*`: read/inspection commands.
- `commands_write.*`: commands that modify the image.

### `images/`
EXT4 test images used to run and validate the shell:

- `myext4image1k.img`
- `myext4image2k.img`
- `myext4image4k.img`

### `checksum/`
Auxiliary checksum code (Crypto++) included in the build.

### Project root

- `main.cpp`: entry point; validates arguments and starts the shell.
- `Makefile`: builds and links the `ext4_app` executable.

## Build and Run

## 1) Dependency

On Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y g++ make libcrypto++-dev
```

## 2) Compile

```bash
make clean
make
```

Generated executable:

- `./ext4_app`

## 3) Run

```bash
./ext4_app images/myext4image4k.img
```

## Available Commands

### Read / Navigation

- `info`: shows general superblock/context information.
- `ls`: lists entries in the current directory.
- `pwd`: prints the current path.
- `cd <dir>`: changes directory (`cd /`, `cd ..`, `cd .`).
- `cat <file>`: prints file contents.
- `attr <file|dir>`: shows attributes (inode, size, mode, links, uid/gid, mtime).
- `testi <inode>`: reports whether an inode is used in the inode bitmap.
- `testb <block>`: reports whether a block is used in the block bitmap.
- `export <source> <host_destination>`: exports a file from the image to the host filesystem.

### Write

- `touch <file>`: creates an empty file.
- `mkdir <dir>`: creates a directory and initializes `.` and `..`.
- `rm <file>`: removes a file (does not remove directories).
- `rmdir <dir>`: removes an empty directory.
- `rename <current_name> <new_name>`: renames an entry in the current directory.

### Session

- `exit` or `quit`: exits the shell.

## Internal Flow (Summary)

1. `main.cpp` calls `ext4_context_init` with the image path.
2. `ext4/context.cpp`:
- opens the image in read/write mode (`O_RDWR`),
- reads the superblock (offset 1024),
- validates magic `0xEF53`,
- validates extents feature,
- computes block size,
- reads block group descriptors,
- loads block and inode bitmaps,
- initializes current directory at root (`inode 2`).
3. Shell (`shell/shell.cpp`) receives commands and dispatches to handlers.
4. Handlers in `commands_read.cpp` and `commands_write.cpp` use the `ext4/` layer APIs.
5. File/directory data reading uses extents in `ext4/inode.cpp` and `ext4/dir.cpp`.

## Usage Examples with Images

## Example A: basic inspection (`myext4image1k.img`)

```bash
./ext4_app images/myext4image1k.img
```

In the shell:

```text
info
pwd
ls
attr .
```

Goal: verify that the image opens correctly, inspect blocks/inodes, and navigate from root.

## Example B: navigation and reading (`myext4image2k.img`)

```bash
./ext4_app images/myext4image2k.img
```

In the shell:

```text
ls
cd <some_directory>
pwd
ls
cat <some_text_file>
export <some_text_file> /tmp/copy.txt
```

Goal: test path resolution, content reading, and host export.

## Example C: write operations (`myext4image4k.img`)

```bash
./ext4_app images/myext4image4k.img
```

In the shell:

```text
mkdir teste
cd teste
touch file.txt
ls
cd ..
rename teste data
ls
rmdir data
```

Goal: validate directory entry creation/rename/removal and metadata updates.

## Important note about write operations

Because write commands modify the image, prefer testing on copies of the original images.

Example:

```bash
cp images/myext4image4k.img images/myext4image4k_test.img
./ext4_app images/myext4image4k_test.img
```

## Input and Output Handling

- Argument validation in `main` and in shell commands.
- Clear error messages for invalid paths, incorrect types, and lack of space.
- Safe numeric parsing in commands such as `testi` and `testb`.
- Return-value checks for `open`, `lseek`, `read`, `write`, and parsing failures.

## Known Limitations

- No directory hash tree support.
- Simplified EXT4 scope for academic use.
- Advanced operations (for example journal handling and complex metadata cases) are out of scope.

## Download of the images

You can download the images from the following links:
- [images.zip](https://drive.google.com/file/d/1jb_fkTmUlh4B7hQo95FS2XMAO7xPQgh3/view?usp=sharing)

## Summary

This project provides a functional and didactic base for byte-level EXT4 exploration,
focused on extents, shell commands, and direct handling of essential filesystem structures.
