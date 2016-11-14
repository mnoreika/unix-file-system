#include "fs.h"

#define MY_MAX_PATH 100
#define MAX_ENTRY_SIZE 10
#define MAX_PATH_SIZE 255
#define MAX_NAME_SIZE 255
#define MAX_BLOCK_SIZE 4
#define MAX_BLOCK_NUMBER 12
#define FIRST_INDIRECT_ENTRY_NUMBER 1024
#define MAX_FILE_SIZE MAX_BLOCK_NUMBER * FIRST_INDIRECT_ENTRY_NUMBER * MAX_BLOCK_SIZE

// Index node data struct, which contains meta information about the file
typedef struct inode_struct {
	uuid_t id; /* unique id of the current file */
	uuid_t data_id; /* unique file data id */

	uid_t  uid;		/* user */
    gid_t  gid;		/* group */
	mode_t mode;	/* protection */
	time_t atime;   /* time of last access */
	time_t mtime;	/* time of last modification */
	time_t ctime;	/* time of last change to meta-data (status) */
	off_t size;		/* size of the data */

} i_node;

// Directory file control block, which contains a key to the targeted directory and its entries
typedef struct dir_fcb {
	uuid_t id;
	
	char entryNames[MAX_ENTRY_SIZE][MAX_NAME_SIZE];
	uuid_t entryIds[MAX_ENTRY_SIZE];

} dir_fcb;


// Data structures that describe storage of files
typedef struct fcb {
	uuid_t direct_blocks[MAX_BLOCK_NUMBER];
	uuid_t single_indirect_blocks;

} fcb;


typedef struct {
	uint8_t data[MAX_BLOCK_SIZE];

} data_block;


typedef struct {
	uuid_t blocks[1024];

} single_indirect;
