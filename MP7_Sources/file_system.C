/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    disk = NULL;
    size = 0;
    inodes = NULL;
    free_blocks = NULL;
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    disk->write(0, (unsigned char *) inodes);
    disk->write(1, free_blocks);
    delete inodes;
    delete free_blocks;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");

    /* Here you read the inode list and the free list into memory */
    disk = _disk;
    size = disk->size();
    // process inodes
    unsigned char * inode_list = new unsigned char [SimpleDisk::BLOCK_SIZE];
    memset(inode_list, 0, SimpleDisk::BLOCK_SIZE);
    disk->read(0, inode_list);
    inodes = (Inode *) inode_list;
    // process free blocks map
    free_blocks = new unsigned char[SimpleDisk::BLOCK_SIZE];
    disk->read(1, free_blocks);
    free_blocks[0] = 1;
    free_blocks[1] = 1;
    delete inode_list;
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    unsigned char empty [SimpleDisk::BLOCK_SIZE];
    memset(empty, 0, SimpleDisk::BLOCK_SIZE);
    _disk->write(0, empty);
    _disk->write(1, empty);
    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    for(int i = 0; i < MAX_INODES; i++) {
        if(inodes[i].id == _file_id) return &inodes[i]; // return address of inode
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    if(LookupFile(_file_id) != NULL)return false;
    for(int i = 0 ; i < MAX_INODES; i++) {
        if(inodes[i].id == 0) {
            //if(GetFreeBlock()== NULL) return false;
            inodes[i].id = _file_id;
            inodes[i].fs = this;
            inodes[i].size = 0;
            inodes[i].block_id = GetFreeBlock();
            return true;
        }
    }
    return false;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */
    for(int i = 0 ; i < MAX_INODES; i++) {
        if(inodes[i].id == _file_id) {
            inodes[i].id = 0;
            inodes[i].fs = NULL;
            inodes[i].size = 0;
            inodes[i].block_id = 0;
            free_blocks[inodes[i].block_id] = 0;
            // clean block
            unsigned char empty [SimpleDisk::BLOCK_SIZE];
            memset(empty, (unsigned char) 0, SimpleDisk::BLOCK_SIZE);
            disk->write(inodes[i].block_id, empty);
            return true;
        }
    }
    return false;
}

int FileSystem::GetFreeBlock() {
    for(int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {
        if(free_blocks[i] == 0) {
            free_blocks[i] = 1;
            return i;
        }
    }
    return NULL;
}
