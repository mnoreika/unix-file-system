/*
  MyFS. One directory, one file, 1000 bytes of storage. What more do you need?

  This Fuse file system is based largely on the HelloWorld example by Miklos Szeredi <miklos@szeredi.hu> (http://fuse.sourceforge.net/helloworld.html). Additional inspiration was taken from Joseph J. Pfeiffer's "Writing a FUSE Filesystem: a Tutorial" (http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/).
*/

#define FUSE_USE_VERSION 26
#define ARROW "->"

#include <fuse.h>
#include <errno.h>
#include <fcntl.h>

#include "myfs.h"

// Storing root directory in memory
i_node root_node;

// Get file and directory attributes (meta-data).
// Read 'man 2 stat' and 'man 2 chmod'. IMPLEMENT
static int myfs_getattr(const char *path, struct stat *stbuf) {
	write_log("myfs_getattr(path=\"%s\", statbuf=0x%08x)\n", path, stbuf);

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = root_node.mode;
		stbuf->st_nlink = 2;
		stbuf->st_uid = root_node.uid;
		stbuf->st_gid = root_node.gid;
	} 
	else {

		if (strstr(path, "/") == 0) {
			write_log("READING DIRECTORY ATTR", path, stbuf);
			stbuf->st_mode = root_node.mode;
			stbuf->st_nlink = 1;
			stbuf->st_mtime = root_node.mtime;
			stbuf->st_ctime = root_node.ctime;
			stbuf->st_size = root_node.size;
			stbuf->st_uid = root_node.uid;
			stbuf->st_gid = root_node.gid;
		}else{
			write_log("myfs_getattr - ENOENT");
			return -ENOENT;
		}
	}

	return 0;
}

// Read a directory.
// Read 'man 2 readdir'. IMPLEMENT
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	(void) offset;
	(void) fi;

	write_log("write_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n", path, buf, filler, offset, fi);

	// This implementation supports only a root directory so return an error if the path is not '/'.
	if (strcmp(path, "/") != 0){
		write_log("myfs_readdir - ENOENT");
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	char *pathP = (char*)&(root_node.path);
	if(*pathP!='\0'){
		// drop the leading '/';
		pathP++;
		filler(buf, pathP, NULL, 0);
	}

	return 0;
}

// Read a file.
// Read 'man 2 read'.
static int myfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	// size_t len;
	// (void) fi;

	// write_log("myfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

	// if(strcmp(path, root_node.path) != 0){
	// 	write_log("myfs_read - ENOENT");
	// 	return -ENOENT;
	// }

	// len = root_node.size;

	// uint8_t data_block[MY_MAX_FILE_SIZE];

	// memset(&data_block, 0, MY_MAX_FILE_SIZE);
	// uuid_t *data_id = &(root_node.data_id);
	// // Is there a data block?
	// if(uuid_compare(zero_uuid,*data_id)!=0){
	// 	unqlite_int64 nBytes;  //Data length.
	// 	int rc = unqlite_kv_fetch(pDb,data_id,KEY_SIZE,NULL,&nBytes);
	// 	if( rc != UNQLITE_OK ){
	// 	  error_handler(rc);
	// 	}
	// 	if(nBytes!=MY_MAX_FILE_SIZE){
	// 		write_log("myfs_read - EIO");
	// 		return -EIO;
	// 	}

	// 	// Fetch the fcb the root data block from the store.
	// 	unqlite_kv_fetch(pDb,data_id,KEY_SIZE,&data_block,&nBytes);
	// }

	// if (offset < len) {
	// 	if (offset + size > len)
	// 		size = len - offset;
	// 	memcpy(buf, &data_block + offset, size);
	// } else
	// 	size = 0;

	// return size;
}

// This file system only supports one file. Create should fail if a file has been created. Path must be '/<something>'.
// Read 'man 2 creat'.
static int myfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
 //    write_log("myfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

 //    if(root_node.path[0] != '\0'){
	// 	write_log("myfs_create - ENOSPC");
	// 	return -ENOSPC;
	// }

	// int pathlen = strlen(path);
	// if(pathlen>=MY_MAX_PATH){
	// 	write_log("myfs_create - ENAMETOOLONG");
	// 	return -ENAMETOOLONG;
	// }
	// sprintf(root_node.path,path);
	// struct fuse_context *context = fuse_get_context();
	// root_node.uid=context->uid;
	// root_node.gid=context->gid;
	// root_node.mode=mode|S_IFREG;

	// int rc = unqlite_kv_store(pDb,&(root_object.id),KEY_SIZE,&root_node,sizeof(i_node));
	// if( rc != UNQLITE_OK ){
	// 	write_log("myfs_create - EIO");
	// 	return -EIO;
	// }

    return 0;
}

// Set update the times (actime, modtime) for a file. This FS only supports modtime.
// Read 'man 2 utime'.
static int myfs_utime(const char *path, struct utimbuf *ubuf){
    write_log("myfs_utime(path=\"%s\", ubuf=0x%08x)\n", path, ubuf);

	if(strcmp(path, root_node.path) != 0){
		write_log("myfs_utime - ENOENT");
		return -ENOENT;
	}
	root_node.mtime=ubuf->modtime;

	// Write the fcb to the store.
    int rc = unqlite_kv_store(pDb,&(root_object.id),KEY_SIZE,&root_node,sizeof(i_node));
	if( rc != UNQLITE_OK ){
		write_log("myfs_write - EIO");
		return -EIO;
	}

    return 0;
}

// Write to a file.
// Read 'man 2 write'
static int myfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
 //    write_log("myfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

	// if(strcmp(path, root_node.path) != 0){
	// 	write_log("myfs_write - ENOENT");
	// 	return -ENOENT;
 //    }

	// if(size >= MY_MAX_FILE_SIZE){
	// 	write_log("myfs_write - EFBIG");
	// 	return -EFBIG;
	// }

	// uint8_t data_block[MY_MAX_FILE_SIZE];

	// memset(&data_block, 0, MY_MAX_FILE_SIZE);
	// uuid_t *data_id = &(root_node.data_id);
	// // Is there a data block?
	// if(uuid_compare(zero_uuid,*data_id)==0){
	// 	// GEnerate a UUID fo rhte data blocl. We'll write the block itself later.
	// 	uuid_generate(root_node.data_id);
	// }else{
	// 	// First we will check the size of the obejct in the store to ensure that we won't overflow the buffer.
	// 	unqlite_int64 nBytes;  // Data length.
	// 	int rc = unqlite_kv_fetch(pDb,data_id,KEY_SIZE,NULL,&nBytes);
		// if( rc!=UNQLITE_OK || nBytes!=MY_MAX_FILE_SIZE){
		// 	write_log("myfs_write - EIO");
		// 	return -EIO;
		// }

	// 	// Fetch the data block from the store.
	// 	unqlite_kv_fetch(pDb,data_id,KEY_SIZE,&data_block,&nBytes);
	// 	// Error handling?
	// }

	// // Write the data in-memory.
 //    int written = snprintf(data_block, MY_MAX_FILE_SIZE, buf);

	// // Write the data block to the store.
	// int rc = unqlite_kv_store(pDb,data_id,KEY_SIZE,&data_block,MY_MAX_FILE_SIZE);
	// if( rc != UNQLITE_OK ){
	// 	write_log("myfs_write - EIO");
	// 	return -EIO;
	// }

	// // Update the fcb in-memory.
	// root_node.size=written;
	// time_t now = time(NULL);
	// root_node.mtime=now;
	// root_node.ctime=now;

	// // Write the fcb to the store.
 //    rc = unqlite_kv_store(pDb,&(root_object.id),KEY_SIZE,&root_node,sizeof(i_node));
	// if( rc != UNQLITE_OK ){
	// 	write_log("myfs_write - EIO");
	// 	return -EIO;
	// }

    return 0; //return written
}

// Set the size of a file.
// Read 'man 2 truncate'.
int myfs_truncate(const char *path, off_t newsize){
 //    write_log("myfs_truncate(path=\"%s\", newsize=%lld)\n", path, newsize);

	// if(newsize >= MY_MAX_FILE_SIZE){
	// 	write_log("myfs_truncate - EFBIG");
	// 	return -EFBIG;
	// }

	// root_node.size = newsize;

	// // Write the fcb to the store.
 //    int rc = unqlite_kv_store(pDb,&(root_object.id),KEY_SIZE,&root_node,sizeof(i_node));
	// if( rc != UNQLITE_OK ){
	// 	write_log("myfs_write - EIO");
	// 	return -EIO;
	// }

	return 0;
}

// Set permissions.
// Read 'man 2 chmod'.
int myfs_chmod(const char *path, mode_t mode){
    write_log("myfs_chmod(fpath=\"%s\", mode=0%03o)\n", path, mode);

    return 0;
}

// Set ownership.
// Read 'man 2 chown'.
int myfs_chown(const char *path, uid_t uid, gid_t gid){
    write_log("myfs_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);

    return 0;
}

// Create a directory IMPLEMENT
int myfs_mkdir(const char *path, mode_t mode) {
	write_log("myfs_mkdir: %s\n",path);

	int pathlen = strlen(path);

	// Returning an error if the directory path is too long
	if (pathlen >= MAX_PATH_SIZE) {
		write_log("myfs_create - ENAMETOOLONG");
		return -ENAMETOOLONG;
	}

	// Find directory that is the parent directory ! IMPLEMENT !
	i_node parent = root_node;

	// Creating an inode for the next directory
	i_node new_dir;
	uuid_generate(new_dir.id);

	// Getting context of the current environment
	struct fuse_context *context = fuse_get_context();
	
	// Setting the context parameters for the new directory
	new_dir.uid = context->uid;
	new_dir.gid = context->gid;
	new_dir.mode = mode | S_IFDIR;

	// Storing the inode of the new directory in the database
	int rc = unqlite_kv_store(pDb, new_dir.id, KEY_SIZE, &new_dir, sizeof(i_node));

	if( rc != UNQLITE_OK ) {
		write_log("myfs_create - EIO");
		return -EIO;
	}

	// Adding the new directory as an entry in the parent directory
	unqlite_int64 nBytes;
	rc = unqlite_kv_fetch (pDb, parent.data_id, KEY_SIZE, NULL, &nBytes);

	if(rc != UNQLITE_OK || nBytes != sizeof(dir_fcb)){
			write_log("myfs_write - EIO");
			return -EIO;
	}

	dir_fcb parent_fcb;  
	rc = unqlite_kv_fetch (pDb, parent.data_id, KEY_SIZE, &parent_fcb, &nBytes);

	for (int i = 0; i < MAX_ENTRY_SIZE; i++) {
		if (strcmp("", parent_fcb.entryNames[i]) == 0) {
			strcpy(parent_fcb.entryNames[i], "directoryName");
			uuid_copy(new_dir.id, parent_fcb.entryIds[i]);
		}
	}

	rc = unqlite_kv_store(pDb, parent.data_id, KEY_SIZE, &parent_fcb, sizeof(parent_fcb));

	if( rc != UNQLITE_OK ) {
		write_log("myfs_create - EIO");
		return -EIO;
	}

    return 0;
}

// Delete a file.
// Read 'man 2 unlink'.
int myfs_unlink(const char *path){
	write_log("myfs_unlink: %s\n",path);

    return 0;
}

// Delete a directory.
// Read 'man 2 rmdir'. IMPLEMENT
int myfs_rmdir(const char *path){
    write_log("myfs_rmdir: %s\n",path);

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
	.mkdir = myfs_mkdir,

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
		root_node.mode |= S_IFDIR|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
		root_node.mtime = time(0);
		root_node.uid = getuid();
		root_node.gid = getgid();

		// Generate a key for root_node and update the root object.
		uuid_generate(root_object.id);

		// Initialise and store the directory fcb
		uuid_generate(root_node.data_id);

		// Make new directory fcb 
		dir_fcb entries;

		uuid_copy(entries.id, root_node.data_id);

		memset(&entries, 0, sizeof(entries));

		// Store directory fcb in the database
		printf("init_fs: writing root entries fcb\n");
		rc = unqlite_kv_store (pDb, &(entries.id), KEY_SIZE, &entries, sizeof(entries));
		if( rc != UNQLITE_OK ){
   			error_handler(rc);
		}
		

		printf("init_fs: writing root fcb\n");
		rc = unqlite_kv_store(pDb, &(root_object.id), KEY_SIZE, &root_node, sizeof(i_node));
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
