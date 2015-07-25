/* 
 * File:   ramdisk.h
 * Author: abhijitmk
 *
 * Created on December 4, 2014, 3:06 PM
 */

#ifndef RAMDISK_H
#define	RAMDISK_H

#include<map>
#include<list>
#include<string>

# define DIR_SIZE 0
# define FILE_SIZE 0
# define MODE_FILE 0444
# define MODE_DIR 0755

# define TYPE_FILE 0
# define TYPE_DIR 1


using namespace std;


typedef struct _ramdisk_file_structure{
    int id;
    char *name;
    int type; // DIR or FILE
    int size;
    char *data;
   
    list<int> children;
    
    time_t atime; // last accessed time
    time_t mtime; // last modified time
    time_t ctime; // created time
    uid_t uid;
    gid_t gid;
    mode_t mode;
    
}ramdisk_file;



#endif	/* RAMDISK_H */

