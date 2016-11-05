#include "fs.h"

#define MY_MAX_PATH 100
#define MY_MAX_FILE_SIZE 1000

// Index node data structure, which contains meta information about the file
typedef struct inode_struct {
	uuid_t data_id; /* unique file data id */

	char path[255]; 	/* path to the file */

	uid_t  uid;		/* user */
    gid_t  gid;		/* group */
	mode_t mode;	/* protection */
	time_t atime;   /* time of last access */
	time_t mtime;	/* time of last modification */
	time_t ctime;	/* time of last change to meta-data (status) */
	off_t size;		/* size */

} i_node;



// // Old version
// struct myfcb{
// 	char path[255];
// 	uuid_t file_data_id;
	
// 	// see 'man 2 stat' and 'man 2 chmod'
// 	//meta-data for the 'file'
// 	uid_t  uid;		/* user */
//   	gid_t  gid;		/* group */
// 	mode_t mode;	/* protection */
// 	time_t mtime;	/* time of last modification */
// 	time_t ctime;	/* time of last change to meta-data (status) */
// 	off_t size;		/* size */
	
// 	//meta-data for the root thing (directory)
// 	uid_t  root_uid;		/* user */
//     gid_t  root_gid;		/* group */
// 	mode_t root_mode;	/* protection */
// 	time_t root_mtime;	/* time of last modification */
// };