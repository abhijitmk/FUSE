/* 
 * File:   ramdisk.cpp
 * Author: abhijitmk
 *
 * Created on December 4, 2014, 1:51 PM
 */

#include <cstdlib>

#include <iostream>
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <map>
#include "ramdisk.h"
#include <libgen.h>
#include <list>

int id;
map<string, int> path_map; // map that has path as key and entry number as value
map<int, ramdisk_file*> inode_map; // map that has entry number as key and the pointer to the ramdisk file as the value
int max_size;
int disk_size = 0;

using namespace std;

// method to get the attributes of the file

static int ramdisk_getattr(const char *path, struct stat *stbuf) {
    int res = 0, tempid;

    memset(stbuf, 0, sizeof (struct stat));

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);


    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];

        stbuf->st_uid = temp_file->uid;
        stbuf->st_gid = temp_file->gid;
        stbuf->st_atime = temp_file->atime;
        stbuf->st_mtime = temp_file->mtime;
        stbuf->st_ctime = temp_file->ctime;

        if (temp_file->type == TYPE_DIR) {
            stbuf->st_mode = S_IFDIR | temp_file->mode;
            stbuf->st_nlink = 2;
            stbuf->st_size = DIR_SIZE;
        } else if (temp_file->type == TYPE_FILE) {
            stbuf->st_mode = S_IFREG | temp_file->mode;
            stbuf->st_nlink = 1;
            stbuf->st_size = temp_file->size;
        }
    }



    return res;
}

// method to read the directory

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {

    int res = 0;
    int tempid;
    struct stat st;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];
        if (temp_file->type != TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else {


            list<int>::const_iterator it;
            //            cout<<temp_file->children.size();
            //            cout<<"\n";
            for (it = temp_file->children.begin(); it != temp_file->children.end(); ++it) {
                
                ramdisk_file *temp_file2 = inode_map[*it];
                char *temp = strdup(temp_file2->name);
                char *basedir = basename(temp);


                filler(buf, basedir, NULL, 0);
                delete temp;

               

            }
        }
        return res;
    }




}

// method to open a file

static int ramdisk_open(const char *path, struct fuse_file_info *fi) {

    int res = 0, tempid;


    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];

        if (temp_file->type == TYPE_DIR) {
            res = -ENOENT;
            return res;
        }

    }
    return res;
}

// method to read a file

static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    int tempid;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        size = -ENOENT;
        return size;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];
        if (temp_file->type == TYPE_DIR) {
            size = -ENOENT;
            return size;
        } else {
            if (temp_file->size - offset <= size)
                size = temp_file->size - offset;


            memcpy(buf, temp_file->data + offset, size);
        }
    }



    return size;
}

// method to write to a file starting from given offset 

int ramdisk_write(const char * path, const char * buf, size_t size, off_t offset, struct fuse_file_info * fi) {


    int tempid;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        size = -ENOENT;
        return size;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];
        if (temp_file->type == TYPE_DIR) {
            size = -ENOENT;
            return size;
        } else {

            if (disk_size + size > max_size) {
                size = -ENOSPC;
                return size;
            }



            if (temp_file->data == NULL) {
                temp_file->data = (char*) malloc(size);
                //memset(temp_file->data,0,size+1);
                memcpy(temp_file->data, buf, size);

            }
            else {
                temp_file->data = (char*) realloc(temp_file->data, offset + size);
                //memset(temp_file->data+offset,0,size);
                memcpy(temp_file->data + offset, buf, size);

            }

            disk_size = disk_size + offset + size - temp_file->size;
            temp_file->size = offset + size;

        }
    }

    return size;
}

// method to make a directory

static int ramdisk_mkdir(const char* path, mode_t mode) {

    int res = 0;
    int tempid;
    struct stat st;

    if (path == NULL) {

        return -ENOENT;
    }

    char *dirc = strdup(path);
    char *basec = strdup(path);
    char *dname = dirname(dirc);
    char *fname = basename(basec);

    string dirname(dname);
    string fullpath(path);

    if (path_map.count(dirname) != 1) {

        res = -ENOENT;
        return res;
    } else {
        tempid = path_map[dirname];
        ramdisk_file *parent_dir = inode_map[tempid];
        if (parent_dir->type != TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else {
            ramdisk_file *newdir;
            newdir = new ramdisk_file;

            int calc_size = disk_size + sizeof (ramdisk_file) + strlen(fname) + sizeof (int);

            // sizeof int is for addition to list of parent

            if (calc_size > max_size) {
                res = -ENOSPC;
                return res;
            }

            path_map[fullpath] = id;

            parent_dir->children.push_back(id);

            newdir->id = id;
            newdir->type = TYPE_DIR;
            newdir->size = DIR_SIZE;
            newdir->name = strdup(fname);
            newdir->gid = fuse_get_context()->gid;
            newdir->uid = fuse_get_context()->uid;
            time(&newdir->atime);
            time(&newdir->mtime);
            time(&newdir->ctime);
            newdir->mode = S_IFDIR | mode;
            newdir->data = NULL;

            inode_map[id] = newdir;

            id++;

            disk_size = calc_size;

            // sizeof int is for addition to list of parent

            
        }
    }

    return res;
}

// method to delete a directory

int ramdisk_rmdir(const char * path) {
    int res = 0;
    int tempid;


    if (path == NULL) {

        return -ENOENT;
    }

    char *pathdup = strdup(path);
    string fullpath(pathdup);

    char *dirc = strdup(pathdup);


    if (path_map.count(fullpath) != 1) {

        res = -ENOENT;
        return res;
    } else {
        tempid = path_map[fullpath];
        ramdisk_file *curr_dir = inode_map[tempid];
        if (curr_dir->type != TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else if (!curr_dir->children.empty()) {
            res = -EPERM;
            return res;
        } else {
            // need to delete entry in child of parent
            // need to remove entry from the maps
            // need to free the memory
            char *parentdirname = dirname(dirc);

            int tempid2 = path_map[parentdirname];
            ramdisk_file *parent_dir = inode_map[tempid2];

            parent_dir->children.remove(tempid);

            inode_map.erase(tempid);
            path_map.erase(fullpath);

            int struct_size = sizeof (ramdisk_file);
            int name_size = strlen(curr_dir->name);


            
            free(curr_dir->name);
            free(curr_dir->data);
            delete curr_dir;

            disk_size = disk_size - (struct_size + name_size + sizeof (int));

            //sizeof(int) is for removal from list of parent

        }
    }

    return res;

}

// method to open a directory

int ramdisk_opendir(const char * path, struct fuse_file_info * fi) {
    int res = 0, tempid;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);


    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];

        if (temp_file->type == TYPE_FILE) {
            res = -ENOENT;
            return res;
        }

    }
    return res;
}

// method to remove a soft link

int ramdisk_unlink(const char * path) {
    int res = 0;
    int tempid;


    if (path == NULL) {

        return -ENOENT;
    }

    char *pathdup = strdup(path);
    string fullpath(pathdup);

    char *dirc = strdup(pathdup);


    if (path_map.count(fullpath) != 1) {

        res = -ENOENT;
        return res;
    } else {
        tempid = path_map[fullpath];
        ramdisk_file *curr_file = inode_map[tempid];
        if (curr_file->type != TYPE_FILE) {
            res = -ENOENT;
            return res;
        }
        else {
            // need to delete entry in child of parent
            // need to remove entry from the maps
            // need to free the memory
            char *parentdirname = dirname(dirc);

            int tempid2 = path_map[parentdirname];
            ramdisk_file *parent_dir = inode_map[tempid2];

            parent_dir->children.remove(tempid);

            inode_map.erase(tempid);
            path_map.erase(fullpath);


            int temp_size = curr_file->size;
            int struct_size = sizeof (ramdisk_file);
            int name_size = strlen(curr_file->name);

           

            free(curr_file->name);
            free(curr_file->data);
            delete curr_file;

            disk_size = disk_size - (temp_size + struct_size + name_size + sizeof (int));

            //sizeof(int) is for removal from list of parent
        }
    }

    return res;

}

// method to create a file

static int ramdisk_create(const char * path, mode_t mode, struct fuse_file_info * fi) {
  
    int res = 0;
    int tempid;
    struct stat st;

    if (path == NULL) {

        return -ENOENT;
    }

    char *dirc = strdup(path);
    char *basec = strdup(path);
    char *dname = dirname(dirc);
    char *fname = basename(basec);
    string dirname(dname);
    string fullpath(path);

    if (path_map.count(dirname) != 1) {

        res = -ENOENT;
        return res;
    } else {
        tempid = path_map[dirname];
        ramdisk_file *parent_dir = inode_map[tempid];
        if (parent_dir->type != TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else {
            ramdisk_file *newfile;
            newfile = new ramdisk_file;

            int calc_size = disk_size + sizeof (ramdisk_file) + strlen(fname) + sizeof (int);

            // sizeof int is for addition to list of parent

            if (calc_size > max_size)
 {
                res = -ENOSPC;
                return res;
            }


            path_map[fullpath] = id;

            parent_dir->children.push_back(id);

            newfile->id = id;
            newfile->type = TYPE_FILE;
            newfile->size = FILE_SIZE;
            newfile->name = strdup(fname);
            newfile->gid = fuse_get_context()->gid;
            newfile->uid = fuse_get_context()->uid;
            time(&newfile->atime);
            time(&newfile->mtime);
            time(&newfile->ctime);
            newfile->mode = S_IFREG | mode;
            newfile->data = NULL;


            inode_map[id] = newfile;

            id++;

            disk_size = calc_size;

            // sizeof int is for addition to list of parent

            
        }
    }

    return res;
}

int ramdisk_flush(const char * path, struct fuse_file_info * fi) {
    int res = 0, tempid;


    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);


    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];

        if (temp_file->type == TYPE_DIR) {
            res = -ENOENT;
            return res;
        }

    }
    return res;
}

int ramdisk_truncate(const char * path, off_t offset) {
    int tempid;
    int res = 0;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];
        if (temp_file->type == TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else {
            int old_filesize = temp_file->size;

            if (offset == 0) {
                free(temp_file->data);

                temp_file->data = NULL;
                temp_file->size = 0;
                disk_size = disk_size - old_filesize;
                return res;
            }

            if (disk_size + offset - old_filesize > max_size) {
                res = -ENOSPC;
                return res;
            }

            temp_file->data = (char*) realloc(temp_file->data, offset);
            temp_file->size = offset;
            disk_size = disk_size + offset - old_filesize;


        }
    }

    return res;
}

// method to truncate a file

int ramdisk_ftruncate(const char * path, off_t offset, struct fuse_file_info * fi) {
    int tempid;
    int res = 0;

    if (path == NULL) {

        return -ENOENT;
    }

    string temp(path);

    if (path_map.count(temp) != 1) {

        res = -ENOENT;
        return res;
    } else {

        tempid = path_map[temp];
        ramdisk_file *temp_file = inode_map[tempid];
        if (temp_file->type == TYPE_DIR) {
            res = -ENOENT;
            return res;
        } else {
            int old_filesize = temp_file->size;
            if (disk_size + offset - old_filesize > max_size) {
                res = -ENOSPC;
                return res;
            }

            temp_file->data = (char*) realloc(temp_file->data, offset);
            temp_file->size = offset;
            disk_size = disk_size + offset - old_filesize;

            if (old_filesize < offset) {
                memset(temp_file->data + old_filesize, 0, offset - old_filesize);

            }

        }
    }

    return res;
}



static struct fuse_operations ramdisk_oper;

/*
 * 
 */
int main(int argc, char** argv) {

    char *params[2];



    if (argc != 3) {
        cout << "USAGE: ./ramdisk <mount point dir> <size of ramdisk(MB)>"<<endl;
        exit(-1);
    }

    max_size = atoi(argv[2])*1024 * 1024;

    if (max_size <= 0) {
        cout << "Invalid size" << endl;
        exit(-1);
    }

    params[0] = argv[0];
    params[1] = argv[1];

  
    ramdisk_oper.getattr = ramdisk_getattr;
    ramdisk_oper.readdir = ramdisk_readdir;
    ramdisk_oper.open = ramdisk_open;
    ramdisk_oper.read = ramdisk_read;
    ramdisk_oper.mkdir = ramdisk_mkdir;
    ramdisk_oper.write = ramdisk_write;
    ramdisk_oper.rmdir = ramdisk_rmdir;
    ramdisk_oper.opendir = ramdisk_opendir;
    ramdisk_oper.unlink = ramdisk_unlink;
    ramdisk_oper.create = ramdisk_create;
    ramdisk_oper.flush = ramdisk_flush;
    ramdisk_oper.truncate = ramdisk_truncate;
   

    id = 1;
    ramdisk_file *root;
    root = new ramdisk_file;

    string r = "/";
    path_map[r] = id;

    root->id = id;
    root->type = TYPE_DIR;
    root->size = DIR_SIZE;
    root->name = (char*) r.c_str();
    root->gid = fuse_get_context()->gid;
    root->uid = fuse_get_context()->uid;
    time(&root->atime);
    time(&root->mtime);
    time(&root->ctime);
    root->mode = 0777;
    root->data = NULL;


    inode_map[id] = root;
    id++;

    disk_size += sizeof (ramdisk_file) + strlen(root->name);

    return fuse_main(argc - 1, params, &ramdisk_oper, NULL);


}

