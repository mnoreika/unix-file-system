#include "fs.h"

#define MY_MAX_PATH 100
#define MY_MAX_FILE_SIZE 1000
#define MAX_ENTRY_SIZE 10
#define MAX_PATH_SIZE 255
#define MAX_NAME_SIZE 255

// Index node data struct, which contains meta information about the file
typedef struct inode_struct {
	uuid_t id; /* unique id of the current file */
	uuid_t data_id; /* unique file data id */

	char path[MAX_PATH_SIZE];

	uid_t  uid;		/* user */
    gid_t  gid;		/* group */
	mode_t mode;	/* protection */
	time_t atime;   /* time of last access */
	time_t mtime;	/* time of last modification */
	time_t ctime;	/* time of last change to meta-data (status) */
	off_t size;		/* size */

} i_node;

// Directory file control block, which contains a key to the targeted directory and its entries
typedef struct dir_fcb {
	uuid_t id;
	
	char entryNames[MAX_ENTRY_SIZE];
	uuid_t entryIds[MAX_ENTRY_SIZE];

} dir_fcb;