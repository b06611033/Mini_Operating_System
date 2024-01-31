/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    pos = 0;
    id = _id;
    fs = _fs;
    inode = _fs->LookupFile(_id);
    size = inode->size;
    fs->disk->read(inode->block_id, block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    fs->disk->write(inode->block_id, block_cache);
    inode->size = size;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    for(int i = 0; i < _n; i++) {
        if(pos < size) {
            _buf[i] = block_cache[pos]; // copy data to buffer
            pos++;
        }
        else {
            Reset();
            return i;
        }
    }
    return _n;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    for(int i = 0; i < _n; i++) {
        if(EoF() == true) {
            Reset();
            return i;
        } 
        else if(pos >= size) {
            size++;
        }
        block_cache[pos] = _buf[i];
        pos++;
    }
    return _n;
}

void File::Reset() {
    Console::puts("resetting file\n");
    pos = 0;
}

bool File::EoF() {
    return pos == SimpleDisk::BLOCK_SIZE;
}
