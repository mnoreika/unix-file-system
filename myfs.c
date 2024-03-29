#define FUSE_USE_VERSION 26
#define ARROW "->"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>

#include "myfs.h"

// Storing root directory in memory
i_node root_node;

char UUID_BUFF[100];

char* get_UUID(uuid_t id)  {
	uuid_unparse(id, UUID_BUFF);
	return UUID_BUFF;
}

// Fetching data from the database
void fetch_data(uuid_t data_id, void* dataStorage, size_t size) {

	// Trying to fetch from the database
	unqlite_int64 nBytes;
	int rc = unqlite_kv_fetch (pDb, data_id, KEY_SIZE, NULL, &nBytes);

	// Handling errors in case of unable to fetch
	if (rc != UNQLITE_OK ) {
			write_log("\nmyfs_database error - cannot fetch data\n");
			error_handler(rc);
	}

	if (nBytes != size) {
		write_log("myfs_database error - fetched data size different than expected");
		exit(-1);
	}

	// Actually fetching the data
	rc = unqlite_kv_fetch (pDb, data_id, KEY_SIZE, dataStorage, &nBytes);
}

// Storing data into the database
void store_data(uuid_t data_id, void* data, size_t size) {
	int rc = unqlite_kv_store(pDb, data_id, KEY_SIZE, data, size);

	// Updating the checked root
	if (uuid_compare(data_id, root_node.id) == 0)
		memcpy(&root_node, data, sizeof(i_node));


	if( rc != UNQLITE_OK ) {
		write_log("\nmyfs_create - storing of the data failed");
		error_handler(rc);
	}
}

// Finding an inode of a target
int findTargetInode(const char* path, i_node* buff) {
	char cp_path[MAX_NAME_SIZE];

	strcpy(cp_path, path);

	char* token = strtok(cp_path, "/");

	i_node current_inode = root_node;
	i_node child_inode = root_node;

	int is_found = 0;
	int tokenCount = 0;

	// Returning root if path has no tokens
	if (token == NULL || strcmp(path, "/") == 0) {
		memcpy(buff, &root_node, sizeof(i_node));
		return 0;
	}

	while (token != NULL) {
		current_inode = child_inode;
		dir_fcb current_fcb;

		fetch_data(current_inode.data_id, &current_fcb, sizeof(dir_fcb));

		for (int i = 0; i < MAX_ENTRY_SIZE; i++) {

			if (strcmp(current_fcb.entryNames[i], token) == 0) {
				i_node next_inode;

				fetch_data(current_fcb.entryIds[i], &next_inode, sizeof(i_node));

				child_inode = next_inode;

				is_found = 1;

				break;
			}
		}

		tokenCount++;

		if (is_found == 0)
			break;

		token = strtok(NULL, "/");
	}

	// When token is 1, current inode is root
	if (is_found == 1 || tokenCount == 1) {
			memcpy(buff, &child_inode, sizeof(i_node));
			return 0;
	}
	else {
		return -1;
	}
}


// Get file and directory attributes (meta-data)
static int myfs_getattr(const char *path, struct stat *stbuf) {
	write_log("\nmyfs_getattr(path=\"%s\", statbuf=0x%08x)\n", path, stbuf);

	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = root_node.mode;
		stbuf->st_nlink = 2;
		stbuf->st_uid = root_node.uid;
		stbuf->st_gid = root_node.gid;
		stbuf->st_ctime = root_node.ctime;
		stbuf->st_atime = root_node.atime;
		stbuf->st_mtime = root_node.mtime;
		stbuf->st_size = root_node.size;

		return 0;
	}
	else {
		i_node parent;



		char *target_name;
		char path_cp[strlen(path) + 1];
		char path_cp2[strlen(path) + 1];

		strcpy(path_cp, path);
		strcpy(path_cp2, path);

		target_name = basename(path_cp);

		char *parentPath;
		parentPath = dirname(path_cp2);



		int found = findTargetInode(parentPath, &parent);

		if (found == -1)
			return -ENOENT;

		dir_fcb parent_dir_fcb;

		fetch_data(parent.data_id, &parent_dir_fcb, sizeof(dir_fcb));

		for (int i = 0; i < MAX_ENTRY_SIZE; i++) {

			if (strcmp(parent_dir_fcb.entryNames[i], "") != 0) {
				write_log("\ngetAttr -> found entry in fcb parent with name: %s", parent_dir_fcb.entryNames[i]);


				if (strcmp(parent_dir_fcb.entryNames[i], target_name) == 0) {
					i_node current;

					fetch_data(parent_dir_fcb.entryIds[i], &current, sizeof(i_node));

					stbuf->st_mode = current.mode;
					stbuf->st_nlink = 2;
					stbuf->st_uid = current.uid;
					stbuf->st_gid = current.gid;
					stbuf->st_ctime = current.ctime;
					stbuf->st_atime = current.atime;
					stbuf->st_mtime = current.mtime;
					stbuf->st_size = current.size;

					return 0;
				}
			}
		}

		write_log("\ngetAttr -> directory not found\n", path, stbuf);

		return -ENOENT;
	}
}

// Read a directory.
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	(void) offset;
	(void) fi;

	write_log("write_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n", path, buf, filler, offset, fi);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	// Finding the parent
	i_node parent;

	findTargetInode(path, &parent);


	dir_fcb parent_dir_fcb;

	fetch_data(parent.data_id, &parent_dir_fcb, sizeof(dir_fcb));

	write_log("\ngetAttr -> current dir name: %s", parent_dir_fcb.entryNames[0]);

	for (int i = 0; i < MAX_ENTRY_SIZE; i++) {
		if (strncmp(parent_dir_fcb.entryNames[i], "", MAX_NAME_SIZE) != 0) {
			filler(buf, parent_dir_fcb.entryNames[i], NULL, 0);
		}


	}

	return 0;
}

int read_single_block(uuid_t block_id, char* buf, size_t size) {
	data_block block;
	fetch_data(block_id, &block, sizeof(data_block));

	int read_size = size;

	if (size > MAX_BLOCK_SIZE)
		read_size = MAX_BLOCK_SIZE;

	memcpy(buf, &block.data, read_size);

	write_log("Data read: %s\n", block.data);

	return read_size;

}


int read_single_indirect_blocks(uuid_t id, char* buf, size_t size, off_t offset) {
	single_indirect indirect_blocks;

	fetch_data(id, &indirect_blocks, sizeof(single_indirect));

	int read_in_total = 0;
	int data_available = size;

	write_log("Reading indirect blocks... ");

	for (int i = 0; i < FIRST_INDIRECT_ENTRY_NUMBER; i++) {
		if (data_available > 0) {

			write_log("Reading indirect block : %d\n", i);
			int read = read_single_block(indirect_blocks.blocks[i], buf + read_in_total, size);

			write_log("Indirect block read: %d\n", i);

			data_available -= read;
			read_in_total += read;

		}
		else {
			write_log("Last read log: %d", i);
			break;
		}

		if (read_in_total >= size)
			break;
	}


	return read_in_total;
}

// Read a file.
static int myfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	(void) fi;

	write_log("\nmyfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

	i_node target;

	if (findTargetInode(path, &target) != 0) {
		write_log("Failed to fetch the target...\n");
	}

	findTargetInode(path, &target);

	fcb target_fcb;

	write_log("Reading the file... \n");

	fetch_data(target.data_id, &target_fcb, sizeof(target_fcb));



	int read_in_total = 0;
	int data_available = target.size;

	for (int i = 0; i < MAX_BLOCK_NUMBER; i++) {

		if (data_available > 0) {

			write_log("Reading block: %d\n", i);
			int read = read_single_block(target_fcb.direct_blocks[i], buf + read_in_total, size);

			write_log("Block read: %d\n", i);

			data_available -= read;
			read_in_total += read;

		}
		else
			break;



		if (read_in_total >= size)
			break;
	}

	if (read_in_total < size && data_available > 0)
		read_in_total += read_single_indirect_blocks(target_fcb.single_indirect_blocks, buf + read_in_total, data_available, offset);


	return read_in_total;
}

static int myfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    write_log("myfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

	int pathlen = strlen(path);

	if (pathlen >= MAX_NAME_SIZE) {
		write_log("myfs_create - ENAMETOOLONG");
		return -ENAMETOOLONG;
	}



	write_log("myfs_create: path - %s\n", path);

	// Getting the inode of the parent
	i_node parent;

	findTargetInode(path, &parent);

	// Fetching the dir fcb of the parent
	dir_fcb parent_fcb;

	fetch_data(parent.data_id, &parent_fcb, sizeof(dir_fcb));

	// Getting the name of the file
	char *file_name;
	char path_cp[strlen(path) + 1];

	strcpy(path_cp, path);

	file_name = basename(path_cp);


	for (int i = 0; i < MAX_ENTRY_SIZE; i++) {
		if (strcmp("", parent_fcb.entryNames[i]) == 0) {
			//Creating a new file and storing it in the parent's fcb
			i_node new_file;

			uuid_generate(new_file.id);

			strcpy(parent_fcb.entryNames[i], file_name);
			uuid_copy(parent_fcb.entryIds[i], new_file.id);

			fcb new_file_fcb;

			memset(&new_file_fcb, 0, sizeof(new_file_fcb));

			uuid_generate(new_file.data_id);

			store_data(new_file.data_id, &new_file_fcb, sizeof(new_file_fcb));

			write_log("\nmyfs_create: file entry has been found and occupied\n");

			write_log("\nmyfs_create: name of the file:  (%s) \n", &parent_fcb.entryNames[i]);

			struct fuse_context *context = fuse_get_context();

			time_t current_time = time(NULL);
			new_file.uid = context->uid;
			new_file.gid = context->gid;
			new_file.mode = mode | S_IFREG;
			new_file.atime = current_time;
			new_file.ctime = current_time;
			new_file.mtime = current_time;
			new_file.size = 0;

			parent.size++;
			parent.mtime = current_time;

			store_data(parent.id, &parent, sizeof(i_node));

			// Storing the file's inode in the database
			store_data(new_file.id, &new_file, sizeof(i_node));

			store_data(parent_fcb.id, &parent_fcb, sizeof(dir_fcb));

			break;
		}
	}

	write_log("\nmyfs_create: file created succesfully\n");


    return 0;
}

// Set update the times (actime, modtime) for a file
static int myfs_utime(const char *path, struct utimbuf *ubuf){
    write_log("myfs_utime(path=\"%s\", ubuf=0x%08x)\n", path, ubuf);


    i_node current;

    findTargetInode(path, &current);

    current.mtime = ubuf->modtime;
    current.atime = ubuf->actime;

	store_data(current.id, &current, sizeof(i_node));

    return 0;
}


// Write data to a single block
int write_to_block(off_t offset, uuid_t block_id, const char *data, size_t size, int new_block) {
	data_block block;

	if (new_block == 1){
		write_log("Block was generated.\n");
		memset(&block, 0, sizeof(data_block));
	}
	else {
		write_log("Block was fetched from the database.\n");
		fetch_data(block_id, &block, sizeof(data_block));
	}

	int to_write = size;
	int written = size;

	if (size >= MAX_BLOCK_SIZE)
		to_write = MAX_BLOCK_SIZE;

	if (offset + size > MAX_BLOCK_SIZE)
		written = MAX_BLOCK_SIZE - offset;

	write_log("Writing to a new block... \n");
	write_log("To write: %d\n", to_write);
	write_log("Writable size:%d\n", to_write);

	snprintf(block.data + offset, to_write + 1, data);

	write_log("Data written: %.4s\n", data);
	write_log("Written to a new block: %d bytes\n", written);

	store_data(block_id, &block, sizeof(data_block));

	return written;
}

// Write data to first indirect blocks
int write_to_indirect_blocks(uuid_t id, size_t size, const char* data, off_t offset, int new_block) {
	single_indirect indirect_blocks;

	write_log("Writting data to single indirect blocks... \n");

	if (new_block == 1)
		memset(&indirect_blocks, 0, sizeof(single_indirect));
	else
		fetch_data(id, &indirect_blocks, sizeof(single_indirect));




	int start_index = (offset / MAX_BLOCK_SIZE);

	int written_in_total = 0;

	int relative_offset = offset - ((offset / MAX_BLOCK_SIZE) * MAX_BLOCK_SIZE);

	if (offset % MAX_BLOCK_SIZE == 0)
		relative_offset = 0;

	write_log("Writting data to single indirect blocks... \n");


	for (int i = start_index; i < FIRST_INDIRECT_ENTRY_NUMBER; i++) {

		write_log("Writting to indirect block: %d\n", i);

		if (i != start_index)
			relative_offset = 0;

		int size_to_write = size - written_in_total;

		int written = 0;
		if (uuid_compare(zero_uuid, indirect_blocks.blocks[i]) == 0) {
			uuid_generate(indirect_blocks.blocks[i]);
			written = write_to_block(relative_offset, indirect_blocks.blocks[i], data, size_to_write, 1);
		}
		else
			written = write_to_block(relative_offset, indirect_blocks.blocks[i], data, size_to_write, 0);



		written_in_total += written;

		data += written;

		if (written_in_total >= size) {
			write_log("Last written log: %d", i);
			break;
		}


	}

	store_data(id, &indirect_blocks, sizeof(single_indirect));

	return written_in_total;
}


// Write to a file.
// Read 'man 2 write'
static int myfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    write_log("\nmyfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

	if (size >= MAX_FILE_SIZE){
		write_log("myfs_write - EFBIG");
		return -EFBIG;
	}

	// Getting the inode of the file
	i_node target;

	findTargetInode(path, &target);

	fcb target_fcb;

	fetch_data(target.data_id, &target_fcb, sizeof(fcb));

	write_log("Writting to file... \n");

	int initial_offset = offset;

	int written_in_total = 0;

	if (offset / MAX_BLOCK_SIZE < MAX_BLOCK_NUMBER) {
		int start_index = (offset / MAX_BLOCK_SIZE);
		int relative_offset = offset - ((offset / MAX_BLOCK_SIZE) * MAX_BLOCK_SIZE);

		if (offset % MAX_BLOCK_SIZE == 0)
			relative_offset = 0;

		for (int i = start_index; i < MAX_BLOCK_NUMBER; i++) {

				if (i != start_index)
					relative_offset = 0;

	            write_log("Printing size: %d\n", size);
	            write_log("Block number: %d\n", i);
	            write_log("Relative offset: %d\n", relative_offset);

	            int size_to_write = size - written_in_total;

				int written = 0;

				//Checking if the block already exists or not
	            if (uuid_compare(zero_uuid, target_fcb.direct_blocks[i]) == 0) {
	            	uuid_generate(target_fcb.direct_blocks[i]);
					written = write_to_block(relative_offset, target_fcb.direct_blocks[i], buf, size_to_write, 1);
				}
				else
					written = write_to_block(relative_offset, target_fcb.direct_blocks[i], buf, size_to_write, 0);

				written_in_total += written;

				write_log("Printing size after written: %d\n", size);
				write_log("Written in total so far:: %d\n", written_in_total);

				buf += written;

				if (written_in_total >= size)
					break;

		}

		// In case all direct blocks are filled
		if (written_in_total < size) {
		  if (uuid_compare(zero_uuid, target_fcb.single_indirect_blocks) == 0) {
          		uuid_generate(target_fcb.single_indirect_blocks);
          		written_in_total += write_to_indirect_blocks(target_fcb.single_indirect_blocks, size - written_in_total, buf, 0, 1);
		  }
		  else
				written_in_total += write_to_indirect_blocks(target_fcb.single_indirect_blocks, size - written_in_total, buf, 0, 0);

		}

	}

	// If offset exceeds the direct block storage capacity
	else {
		offset -= MAX_BLOCK_SIZE * MAX_BLOCK_NUMBER;

		 if (uuid_compare(zero_uuid, target_fcb.single_indirect_blocks) == 0) {
          		uuid_generate(target_fcb.single_indirect_blocks);
          		written_in_total += write_to_indirect_blocks(target_fcb.single_indirect_blocks, size, buf, offset, 1);
		  }
		  else
				written_in_total += write_to_indirect_blocks(target_fcb.single_indirect_blocks, size, buf, offset, 0);
	}


	// Calculating the size of the file
	if (initial_offset + size > target.size)
		target.size += (initial_offset + size) - target.size;

	// Saving the target fcb to the database
	store_data(target.data_id, &target_fcb, sizeof(fcb));

	store_data(target.id, &target, sizeof(i_node));

	write_log("Written in total: %d\n", written_in_total);

	write_log("File written succesfully. \n");

	return written_in_total;
}

// Set the size of a file.
// Read 'man 2 truncate'.
int myfs_truncate(const char *path, off_t newsize){
    write_log("myfs_truncate(path=\"%s\", newsize=%lld)\n", path, newsize);

	if(newsize >= MAX_FILE_SIZE){
		write_log("myfs_truncate - EFBIG");
		return -EFBIG;
	}

	i_node target;

	findTargetInode(path, &target);

	if (target.size >= newsize)
		target.size = newsize;
	else {
		char buf[newsize - target.size];
		memset(buf, 0, newsize - target.size);

		myfs_write(path, buf, newsize - target.size, target.size, NULL);

		target.size += newsize - target.size;
	}

	// Write the inode to the store.
   	store_data(target.id, &target, sizeof(target));

	return 0;
}

// Set permissions.
// Read 'man 2 chmod'.
int myfs_chmod(const char *path, mode_t mode){
    write_log("myfs_chmod(fpath=\"%s\", mode=0%03o)\n", path, mode);

    i_node target;

    findTargetInode(path, &target);

    target.mode = mode;



    store_data(target.id, &target, sizeof(i_node));

    return 0;
}

// Set ownership.
// Read 'man 2 chown'.
int myfs_chown(const char *path, uid_t uid, gid_t gid){
    write_log("myfs_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);

    i_node target;

    findTargetInode(path, &target);

    target.uid = uid;
    target.gid = gid;

    store_data(target.id, &target, sizeof(target));

    return 0;
}

// Create a directory
int myfs_mkdir(const char *path, mode_t mode) {
	write_log("\nmyfs_mkdir : %s\n",path);

	int pathlen = strlen(path);

	// Returning an error if the directory path is too long
	if (pathlen >= MAX_NAME_SIZE) {
		write_log("\nmyfs_create - ENAMETOOLONG");
		return -ENAMETOOLONG;
	}

	// Find directory that is the parent directory
	i_node parent;


	if (findTargetInode(path, &parent) != 0) {
		write_log("myfs_mkdir: parent not found\n");
	}

	char *dirname;
	char *path_cp = strdup(path);

	dirname = basename(path_cp);


	write_log("Base name: %s\n", dirname);


	// Adding the new directory as an entry in the parent directory
	dir_fcb parent_fcb;

	fetch_data(parent.data_id, &parent_fcb, sizeof(dir_fcb));

	write_log("\nmyfs_mkdir: directory created!");

	// Getting directory name

	// Finding an empty entry space
	for (int i = 0; i < MAX_ENTRY_SIZE; i++) {

		if (strcmp("", parent_fcb.entryNames[i]) == 0) {
			// Creating an inode for the next directory
			i_node new_dir;

			uuid_generate(new_dir.id);

			// Creating a dir file control block for the new directory
			dir_fcb new_dir_entries;

			memset(&new_dir_entries, 0, sizeof(dir_fcb));

			uuid_generate(new_dir_entries.id);

			uuid_copy(new_dir.data_id, new_dir_entries.id);

			store_data(new_dir.data_id, &new_dir_entries, sizeof(dir_fcb));

			// Getting context of the current environment
			struct fuse_context *context = fuse_get_context();
			time_t current_time = time(NULL);

			// Setting the context parameters for the new directory
			new_dir.uid = context->uid;
			new_dir.gid = context->gid;
			new_dir.mode = mode | S_IFDIR;
			new_dir.atime = current_time;
			new_dir.ctime = current_time;
			new_dir.mtime = current_time;
			new_dir.size = 0;

			write_log("ID of the dir: %s", get_UUID(new_dir.id));

			// Storing the inode of the new directory in the database
			store_data(new_dir.id, &new_dir, sizeof(i_node));

			strcpy(parent_fcb.entryNames[i], dirname);
			uuid_copy(parent_fcb.entryIds[i], new_dir.id);
			write_log("\nmyfs_mkdir: dir entry has been found and occupied");

			write_log("\nmyfs_mkdir: name of the dir:  %s : %s\n", &parent_fcb.entryNames[i], dirname);

			parent.size++;
			break;
		}

	}

	store_data(parent.data_id, &parent_fcb, sizeof(dir_fcb));

	store_data(parent.id, &parent, sizeof(i_node));

	write_log("\nmyfs_mkdir: directory %s created!", dirname);

    return 0;
}

// Delete a file.
// Read 'man 2 unlink'.
int myfs_unlink(const char *path){
	write_log("myfs_unlink: %s\n",path);


	i_node parent;

	char *parentPath;
	char *target_name;

	char path_cp[strlen(path) + 1];
	char path_cp2[strlen(path) + 1];

	strcpy(path_cp, path);
	strcpy(path_cp2, path);

	parentPath = dirname(path_cp);
	target_name = basename(path_cp2);

	write_log("Target name: %s", target_name);

	findTargetInode(parentPath, &parent);

	dir_fcb parent_fcb;

	fetch_data(parent.data_id, &parent_fcb, sizeof(dir_fcb));

	for (int i = 0; i < MAX_ENTRY_SIZE; i++) {
		write_log("Looping... \n");

		if (strcmp(parent_fcb.entryNames[i], target_name) == 0) {
			write_log("Found data and trying to delete: \n");
			memset(&parent_fcb.entryNames[i], 0, sizeof(char) * MAX_NAME_SIZE);

			parent.size--;
			store_data(parent.data_id, &parent_fcb, sizeof(dir_fcb));
			store_data(parent.id, &parent, sizeof(i_node));
			return 0;
		}
	}

    return -ENOENT;
}

// Delete a directory.
// Read 'man 2 rmdir'. IMPLEMENT
int myfs_rmdir(const char *path) {
    write_log("myfs_rmdir: %s\n",path);


    i_node target;

    findTargetInode(path, &target);

    dir_fcb target_dir_fcb;

    write_log("\nTrying to fetch \n");

    fetch_data(target.data_id, &target_dir_fcb, sizeof(dir_fcb));

    write_log("\nFetch succesfull\n");

    for (int i = 0; i < MAX_ENTRY_SIZE; i++) {

    	if (strcmp("", target_dir_fcb.entryNames[i]) != 0) {
    		return -ENOTEMPTY;

    	}
    }


    myfs_unlink(path);

    return 0;
}

// OPTIONAL - included as an example
// Flush any cached data.
int myfs_flush(const char *path, struct fuse_file_info *fi){
    int retstat = 0;

    write_log("myfs_flush(path=\"%s\", fi=0x%08x)\n", path, fi);

    return retstat;
}

// OPTIONAL - included as an example
// Release the file. There will be one call to release for each call to open.
int myfs_release(const char *path, struct fuse_file_info *fi){
    int retstat = 0;

    write_log("myfs_release(path=\"%s\", fi=0x%08x)\n", path, fi);

    return retstat;
}

// OPTIONAL - included as an example
// Open a file. Open should check if the operation is permitted for the given flags (fi->flags).
// Read 'man 2 open'.
static int myfs_open(const char *path, struct fuse_file_info *fi){
	// if (strcmp(path, root_node.path) != 0)
	// 	return -ENOENT;

	write_log("myfs_open(path\"%s\", fi=0x%08x)\n", path, fi);

	//return -EACCES if the access is not permitted.

	return 0;
}

static struct fuse_operations myfs_oper = {
	.getattr	= myfs_getattr,
	.readdir	= myfs_readdir,
	.open		= myfs_open,
	.read		= myfs_read,
	.create		= myfs_create,
	.utime 		= myfs_utime,
	.write		= myfs_write,
	.truncate	= myfs_truncate,
	.flush		= myfs_flush,
	.release	= myfs_release,
	.mkdir 		= myfs_mkdir,
	.rmdir      = myfs_rmdir,
	.unlink     = myfs_unlink,
	.chmod 		= myfs_chmod,
	.chown      = myfs_chown,

};


// Initialise the in-memory data structures from the store. If the root object (from the store) is empty then create a root fcb (directory)
// and write it to the store. Note that this code is executed outide of fuse. If there is a failure then we have failed to initialise the
// file system so exit with an error code.
void init_fs() {

	int rc;
	printf("Initialising the file system... \n");

	//Initialise the store.
	init_store();

	if (!root_is_empty) {
		printf("%s %s %s", __func__,  ARROW, " Root directory is not empty\n");

		uuid_t *data_id = &(root_object.id);

		unqlite_int64 nBytes;  //Data length.
		rc = unqlite_kv_fetch(pDb,data_id,KEY_SIZE,NULL,&nBytes);

		if( rc != UNQLITE_OK ){
		  error_handler(rc);
		}

		if(nBytes!=sizeof(i_node)){
			printf("Data object has unexpected size. Doing nothing.\n");
			exit(-1);
		}

		//Fetch the fcb that the root object points at
		unqlite_kv_fetch(pDb,data_id,KEY_SIZE,&root_node,&nBytes);

	} else {
		printf("%s %s %s", __func__,  ARROW, " Root directory is empty\n");

		// Initialise and store an empty root inode
		memset(&root_node, 0, sizeof(i_node));

		// See 'man 2 stat' and 'man 2 chmod'.
		time_t current_time = time(NULL);

		root_node.mode |= S_IFDIR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
		root_node.mtime = current_time;
		root_node.atime = current_time;
		root_node.ctime = current_time;
		root_node.uid = getuid();
		root_node.gid = getgid();

		// Generate a key for root_node and update the root object.
		uuid_generate(root_object.id);

		// Initialise and store the directory fcb
		uuid_generate(root_node.data_id);

		// Make new directory fcb
		dir_fcb entries;

		memset(&entries, 0, sizeof(entries));

		uuid_copy(entries.id, root_node.data_id);

		// Store directory fcb in the database
		printf("init_fs: writing root entries fcb\n");
		rc = unqlite_kv_store (pDb, entries.id, KEY_SIZE, &entries, sizeof(entries));
		if( rc != UNQLITE_OK ){
   			error_handler(rc);
		}


		printf("init_fs: writing root fcb\n");
		rc = unqlite_kv_store(pDb, root_object.id, KEY_SIZE, &root_node, sizeof(i_node));
		if( rc != UNQLITE_OK ){
   			error_handler(rc);
		}

		printf("init_fs: writing updated root object\n");

		//Store root object.
		rc = write_root();
	 	if( rc != UNQLITE_OK ){
   			error_handler(rc);
		}
	}
}

void shutdown_fs(){
	unqlite_close(pDb);
}

int main(int argc, char *argv[]){
	int fuserc;
	struct myfs_state *myfs_internal_state;

	//Setup the log file and store the FILE* in the private data object for the file system.
	myfs_internal_state = malloc(sizeof(struct myfs_state));
    myfs_internal_state->logfile = init_log_file();

	//Initialise the file system. This is being done outside of fuse for ease of debugging.
	init_fs();

	fuserc = fuse_main(argc, argv, &myfs_oper, myfs_internal_state);

	//Shutdown the file system.
	shutdown_fs();

	return fuserc;
}
