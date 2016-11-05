#include "fs.h"

#define MAX_PATH 4096
#define MAX_BLOCK_SIZE 

// Index node data structure, which contains meta information about a file
struct inode_struct {
	uuid_t data_id; /* unique file data id */
	
	char path[MAX_PATH]; /* path to the file */

	// Meta-data
	uid_t  uid;		/* user */
    gid_t  gid;		/* group */
	mode_t mode;	/* protection */
	time_t atime;   /* time of last access */
	time_t mtime;	/* time of last modification */
	time_t ctime;	/* time of last change to meta-data (status) */
	off_t size;		/* size */


	
} i_node;