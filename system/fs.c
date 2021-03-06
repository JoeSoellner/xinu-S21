#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

//#ifdef FS
#include <fs.h>

static fsystem_t fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0 // Superblock
#define BM_BLK 1 // Bitmapblock

#define NUM_FD 16

filetable_t oft[NUM_FD]; // open file table
#define isbadfd(fd) (fd < 0 || fd >= NUM_FD || oft[fd].in.id == EMPTY)

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(inode_t))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

/**
 * Helper functions
 */

int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEDIRECTBLOCKS) {
    errormsg("No indirect block support! (%d >= %d)\n", fileblock, INODEBLOCKS - 2);
    return SYSERR;
  }

  // Get the logical block address
  diskblock = oft[fd].in.blocks[fileblock];

  return diskblock;
}

/**
 * Filesystem functions
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out) {
  int bl, inn;
  int inode_off;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %s)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl  = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(inode_t);

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(out, &block_cache[inode_off], sizeof(inode_t));

  return OK;

}

int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in) {
  int bl, inn;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %d)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(inode_t))], in, sizeof(inode_t));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == dev0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  } else {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  } else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) { i++; }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *) SYSERR) {
    errormsg("fs_mkfs memget failed\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i = 0; i < fsd.freemaskbytes; i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(fsystem_t));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  // Initialize all inode IDs to EMPTY
  inode_t tmp_in;
  for (i = 0; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    tmp_in.id = EMPTY;
    _fs_put_inode_by_num(dev0, i, &tmp_in);
  }
  fsd.root_dir.numentries = 0;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  }

  for (i = 0; i < NUM_FD; i++) {
    oft[i].state     = 0;
    oft[i].fileptr   = 0;
    oft[i].de        = NULL;
    oft[i].in.id     = EMPTY;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, INODEBLOCKS);
    oft[i].flag      = 0;
  }

  return OK;
}

int fs_freefs(int dev) {
  if (freemem(fsd.freemask, fsd.freemaskbytes) == SYSERR) {
    return SYSERR;
  }

  return OK;
}

/**
 * Debugging functions
 */
void fs_print_oft(void) {
  int i;

  printf ("\n\033[35moft[]\033[39m\n");
  printf ("%3s  %5s  %7s  %8s  %6s  %5s  %4s  %s\n", "Num", "state", "fileptr", "de", "de.num", "in.id", "flag", "de.name");
  for (i = 0; i < NUM_FD; i++) {
    if (oft[i].de != NULL) printf ("%3d  %5d  %7d  %8d  %6d  %5d  %4d  %s\n", i, oft[i].state, oft[i].fileptr, oft[i].de, oft[i].de->inode_num, oft[i].in.id, oft[i].flag, oft[i].de->name);
  }

  printf ("\n\033[35mfsd.root_dir.entry[] (numentries: %d)\033[39m\n", fsd.root_dir.numentries);
  printf ("%3s  %3s  %s\n", "ID", "id", "filename");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num != EMPTY) printf("%3d  %3d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
  printf("\n");
}

void fs_print_inode(int fd) {
  int i;

  printf("\n\033[35mInode FS=%d\033[39m\n", fd);
  printf("Name:    %s\n", oft[fd].de->name);
  printf("State:   %d\n", oft[fd].state);
  printf("Flag:    %d\n", oft[fd].flag);
  printf("Fileptr: %d\n", oft[fd].fileptr);
  //printf("Id:      %s\n", oft[fd].in.id);
  printf("Type:    %d\n", oft[fd].in.type);
  printf("nlink:   %d\n", oft[fd].in.nlink);
  printf("device:  %d\n", oft[fd].in.device);
  printf("size:    %d\n", oft[fd].in.size);
  printf("blocks: ");
  for (i = 0; i < INODEBLOCKS; i++) {
    printf(" %d", oft[fd].in.blocks[i]);
  }
  printf("\n");
  return;
}

void fs_print_fsd(void) {
  int i;

  printf("\033[35mfsystem_t fsd\033[39m\n");
  printf("fsd.nblocks:       %d\n", fsd.nblocks);
  printf("fsd.blocksz:       %d\n", fsd.blocksz);
  printf("fsd.ninodes:       %d\n", fsd.ninodes);
  printf("fsd.inodes_used:   %d\n", fsd.inodes_used);
  printf("fsd.freemaskbytes  %d\n", fsd.freemaskbytes);
  printf("sizeof(inode_t):   %d\n", sizeof(inode_t));
  printf("INODES_PER_BLOCK:  %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS:  %d\n", NUM_INODE_BLOCKS);

  inode_t tmp_in;
  printf ("\n\033[35mBlocks\033[39m\n");
  printf ("%3s  %3s  %4s  %4s  %3s  %4s\n", "Num", "id", "type", "nlnk", "dev", "size");
  for (i = 0; i < NUM_FD; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) printf("%3d  %3d  %4d  %4d  %3d  %4d\n", i, tmp_in.id, tmp_in.type, tmp_in.nlink, tmp_in.device, tmp_in.size);
  }
  for (i = NUM_FD; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) {
      printf("%3d:", i);
      int j;
      for (j = 0; j < 64; j++) {
        printf(" %3d", *(((char *) &tmp_in) + j));
      }
      printf("\n");
    }
  }
  printf("\n");
}

void fs_print_dir(void) {
  int i;

  printf("%22s  %9s  %s\n", "DirectoryEntry", "inode_num", "name");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    printf("fsd.root_dir.entry[%2d]  %9d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
}

int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/**
 * This is maybe a little overcomplicated since the lowest-numbered
 * block is indicated in the high-order bit.  Shift the byte by j
 * positions to make the match in bit7 (the 8th bit) and then shift
 * that value 7 times to the low-order bit to print.  Yes, it could be
 * the other way...
 */
void fs_printfreemask(void) { // print block bitmask
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    printf(" ");
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

int fs_open(char *filename, int flags) {
	//bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
	//fs_mkfs(0, DEFAULT_NUM_INODES);

	// find entry matching the give filename
	for(int i = 0; i < DIRECTORY_SIZE; i++) {
		dirent_t currEntry = fsd.root_dir.entry[i];

		// kprintf("currEntryName: %s, filename: %s \n", &(currEntry.name[0]), filename);
		// if the filename doesnt match go onto the next file
		if(strncmp(&(currEntry.name[0]), filename, FILENAMELEN) != 0) {
			continue;
		}

		// otherwise the filename does match
		//kprintf("filename match\n");
		int firstSpaceForOpenFile = -1;
		// check if the file is already open
		for(int j = 0; j < NUM_FD; j++) {
			filetable_t currOpenFile = oft[j];

			// assuming that all open files are at beginning and all empty spaces are at the end
			// remember the index of the first open spot
			if(currOpenFile.de == NULL && firstSpaceForOpenFile == -1) {
				firstSpaceForOpenFile = j;
			}

			// file is in open file table with the same name
			if(strncmp(&(currOpenFile.de->name[0]), filename, FILENAMELEN) == 0) {
				// if the file is in the oft and close then just open it and exit function
				if(currOpenFile.state == FSTATE_CLOSED) {
					oft[j].state = FSTATE_OPEN;
					oft[j].flag = flags;
					return j;
				}

				//kprintf("file is already open\n");
				// otherwise file is open and return error
				return SYSERR;
			}
		}

		// no spaces for open file
		if(firstSpaceForOpenFile == -1) {
			//kprintf("no space for the file in open file table\n");
			return SYSERR;
		}

		// create structs to put in open file table
		struct inode newInode;
		newInode.id     = 0;
		newInode.type   = 0;
		newInode.nlink  = 0;
		newInode.device = 0;
		newInode.size   = 0;
		memset(newInode.blocks, EMPTY, INODEBLOCKS*sizeof(*newInode.blocks));
		_fs_get_inode_by_num(dev0, currEntry.inode_num, &newInode);
		fsd.inodes_used += 1;

		struct dirent *openFileDirectoryEntryPtr = (struct dirent*) getmem(sizeof(dirent_t));
		openFileDirectoryEntryPtr->inode_num = currEntry.inode_num;
		strcpy(&(openFileDirectoryEntryPtr->name[0]), filename);

		oft[firstSpaceForOpenFile].state = FSTATE_OPEN;
		oft[firstSpaceForOpenFile].fileptr = 0;
		oft[firstSpaceForOpenFile].de = openFileDirectoryEntryPtr;
		oft[firstSpaceForOpenFile].in = newInode;
		oft[firstSpaceForOpenFile].flag = flags;

		// idk if im returning the right thing
		return firstSpaceForOpenFile;
	}

	//kprintf("file not found\n");
  	return SYSERR;
}

int fs_close(int fd) {
	filetable_t fileToClose = oft[fd];

	if(fileToClose.state == FSTATE_CLOSED) {
		return SYSERR;
	}

	oft[fd].state = FSTATE_CLOSED;
  	return OK;
}

int fs_create(char *filename, int mode) {
	int firstSpaceForOpenEntry = -1;

	// iterate over all the files in the root directory
	for(int i = 0; i < DIRECTORY_SIZE; i++) {
		dirent_t currEntry = fsd.root_dir.entry[i];

		// if a file already exist with this name then return an error
		if(strncmp(&(currEntry.name[0]), filename, FILENAMELEN) == 0) {
			//kprintf("ERROR: file already exists with that name\n");
			return SYSERR;
		}

		// if there is an open index for a file and we don't have an index saved yet
		// then save the index
		if(currEntry.inode_num == EMPTY && firstSpaceForOpenEntry == -1) {
			firstSpaceForOpenEntry = i;
		}
	}

	// there is no open index for a file
	if(firstSpaceForOpenEntry == -1) {
		//kprintf("ERROR: no space for file\n");
		return SYSERR;
	}

	// assuming each inode number is the same as its dirent index number in the root directory????????
	int inodeNum = firstSpaceForOpenEntry;

	// create structs to put in open file table
	// not sure about what to assign exactly???????
	struct inode newInode;
	newInode.id     = inodeNum;
	newInode.type   = INODE_TYPE_FILE;
	newInode.nlink  = 1;
	newInode.device = dev0;
	newInode.size   = 0;
	memset(newInode.blocks, EMPTY, INODEBLOCKS*sizeof(*newInode.blocks));
	_fs_put_inode_by_num(dev0, inodeNum, &newInode);

	struct dirent *openFileDirectoryEntryPtr = (struct dirent*) getmem(sizeof(dirent_t));
	openFileDirectoryEntryPtr->inode_num = inodeNum;
	strcpy(&(openFileDirectoryEntryPtr->name[0]), filename);

	//kprintf("firstSpaceForOpenEntry: %d\n", firstSpaceForOpenEntry);
	fsd.root_dir.numentries += 1;
	fsd.root_dir.entry[firstSpaceForOpenEntry] = *openFileDirectoryEntryPtr;
	//kprintf("firstSpaceForOpenEntry: %d, entryName: %s \n", firstSpaceForOpenEntry, fsd.root_dir.entry[firstSpaceForOpenEntry].name);

	// what do i use for the flags??????
	int openResult = fs_open(filename, O_RDWR);
	//kprintf("openResult: %d\n", openResult);

	return openResult;
}

int fs_seek(int fd, int offset) {
	if(isbadfd(fd)) {
		return SYSERR;
	}
	if(offset < 0 || offset >= oft[fd].in.size || oft[fd].state == FSTATE_CLOSED) {
		return SYSERR;
	}

	oft[fd].fileptr = offset;
  	return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
	if(isbadfd(fd)) {
		return SYSERR;
	}
	if(nbytes < 0) {
		return SYSERR;
	}
	if(oft[fd].flag == O_WRONLY) {
		return SYSERR;
	}

	int numOfBlocksOfInodeUsed = 0;
	// find the number of blocks in use
	for(int i = 0; i < INODEBLOCKS; i++) {
		// this index isnt being used by a block
		if(oft[fd].in.blocks[i] == EMPTY) {
			// assuming all the empty blocks are at the end
			break;
		}
		
		numOfBlocksOfInodeUsed += 1;
	}
	//kprintf("numOfBlocksOfInodeUsed: %d \n", numOfBlocksOfInodeUsed);

	// use an extra pointer to add btyes
	// this way we can do pointer math and buf still points to head
	void *bufferIter = buf;
	for(int i = 0; i < nbytes; i++) {
		// no more space to read from
		if(oft[fd].fileptr > oft[fd].in.size) {
			//kprintf("ERROR: No more space to read from\n");
			return nbytes;
			//return SYSERR;
		}

		// get the location of the filePtr in the current block
		int currBlockOffset = oft[fd].fileptr % MDEV_BLOCK_SIZE;
		int blockIndexInInode = oft[fd].fileptr / MDEV_BLOCK_SIZE;
		
		// read a byte from block into buffer
		//kprintf("block: %d, offset: %d \n", oft[fd].in.blocks[blockIndexInInode], currBlockOffset);
		//char currChar = '1';
		bs_bread(dev0, oft[fd].in.blocks[blockIndexInInode], currBlockOffset, (void *) bufferIter, sizeof(byte));
		//kprintf("numBytesWritten: %d, charRead: %c \n", i + 1, *((char *) bufferIter));
		// increment buffer so we can read next byte
		bufferIter = (char *) bufferIter + 1;

		// update fileptr
		oft[fd].fileptr += 1;
	}

  	return nbytes;
}

int fs_write(int fd, void *buf, int nbytes) {
	if(isbadfd(fd)) {
		return SYSERR;
	}
	if(nbytes < 0) {
		return SYSERR;
	}
	if(oft[fd].flag == O_RDONLY) {
		return SYSERR;
	}

	for(int i = 0; i < nbytes; i++) {
		// need to allocate a new block 
		if(oft[fd].fileptr == oft[fd].in.size) {
			int newBlockIndex = -1;

			for(int j = 0; j < MDEV_NUM_BLOCKS; j++) {
				// this block is in use
				if(fs_getmaskbit(j) == 1) {
					continue;
				}

				// this block is free and were gonna use it
				fs_setmaskbit(j);
				newBlockIndex = j;
				break;
			}

			if(newBlockIndex == -1) {
				//kprintf("ERROR: No more blocks left to allocate\n");
				return i;
			}

			int indexOfNewBlockInInode = -1;
			// place the block in the inodes array of blocks
			for(int j = 0; j < INODEBLOCKS; j++) {
				// this index is being used by a block
				//kprintf("i: %d, value: %d \n", j, oft[fd].in.blocks[j]);
				if(oft[fd].in.blocks[j] != EMPTY) {
					//kprintf("i: %d, value: %d \n", i, oft[fd].in.blocks[i]);
					continue;
				}
				
				// this index is empty so we put our new block in
				oft[fd].in.blocks[j] = newBlockIndex;
				//_fs_put_inode_by_num(dev0, oft[fd].in.id, &(oft[fd].in)); needed ????
				indexOfNewBlockInInode = j;
				break;
			}

			if(indexOfNewBlockInInode == -1) {
				//kprintf("ERROR: Inode does not have any more space for blocks\n");
				return i;
			}

			// add the size of the block (in bytes?) to the file size
			oft[fd].in.size += MDEV_BLOCK_SIZE;
		}

		// get the location of the filePtr in the current block
		int currBlockOffset = oft[fd].fileptr % MDEV_BLOCK_SIZE;
		int blockIndexInInode = oft[fd].fileptr / MDEV_BLOCK_SIZE;
		
		// write a byte from buffer in block
		//kprintf("block: %d, offset: %d \n", oft[fd].in.blocks[blockIndexInInode], currBlockOffset);
		bs_bwrite(dev0, oft[fd].in.blocks[blockIndexInInode], currBlockOffset, buf, sizeof(byte));
		//kprintf("numBytesWritten: %d, charWritten: %c \n", i + 1, *((char *) buf));
		// increment buffer so we can read next byte
		buf = (char *) buf + 1;

		// update fileptr
		oft[fd].fileptr += 1;
	}

	// what if we write some bytes and but encouter an error???
  	return nbytes;
}

int fs_link(char *src_filename, char* dst_filename) {
	int inodeNum = -1;
	int firstSpaceForOpenEntry = -1;

	// iterate over all the files in the root directory
	for(int i = 0; i < DIRECTORY_SIZE; i++) {
		dirent_t currEntry = fsd.root_dir.entry[i];

		// if the file name matches then record the index and break
		// there shouldn't be duplicate names so don't need to check if inodeNum has be assigned yet
		if(strncmp(&(currEntry.name[0]), src_filename, FILENAMELEN) == 0) {
			inodeNum = fsd.root_dir.entry[i].inode_num;
		}

		// if there is an open index for a file and we don't have an index saved yet
		// then save the index
		if(currEntry.inode_num == EMPTY && firstSpaceForOpenEntry == -1) {
			firstSpaceForOpenEntry = i;
		}

		// if there is a fill with the name we are trying to use then error
		if(strncmp(&(currEntry.name[0]), dst_filename, FILENAMELEN) == 0) {
			//kprintf("ERROR: Already a file with that name, no duplicate filenames \n");
			return SYSERR;
		}
	}

	// there is no file with the name we are searching for
	if(inodeNum == -1) {
		//kprintf("ERROR: A file does not exist with that name \n");
		return SYSERR;
	}
	if(firstSpaceForOpenEntry == -1) {
		//kprintf("ERROR: No space for new entry \n");
		return SYSERR;
	}
	
	//printf("inodeNum: %d\n", inodeNum);
	struct inode *inodePtr = (struct inode*) getmem(sizeof(inode_t));
	_fs_get_inode_by_num(dev0, inodeNum, inodePtr);
	//printf("inode.nlinks: %d\n", inodePtr->nlink);
	inodePtr->nlink += 1;

	for (int i = 0; i < NUM_FD; i++) {
		if(oft[i].de->inode_num == inodeNum) {
			oft[i].in.nlink += 1;
		}
  	}

	//printf("inode.nlinks: %d\n", inodePtr->nlink);
	_fs_put_inode_by_num(dev0, inodeNum, inodePtr);
	//_fs_get_inode_by_num(dev0, inodeNum, inodePtr);
	//printf("inode.nlinks: %d\n", inodePtr->nlink);

	struct dirent *openFileDirectoryEntryPtr = (struct dirent*) getmem(sizeof(dirent_t));
	openFileDirectoryEntryPtr->inode_num = inodeNum;
	strcpy(&(openFileDirectoryEntryPtr->name[0]), dst_filename);

	fsd.root_dir.numentries += 1;
	fsd.root_dir.entry[firstSpaceForOpenEntry] = *openFileDirectoryEntryPtr;

	int firstSpaceForOpenFile = -1;
	// check if the file is already open
	for(int i = 0; i < NUM_FD; i++) {
		filetable_t currOpenFile = oft[i];

		// assuming that all open files are at beginning and all empty spaces are at the end
		// remember the index of the first open spot
		if(currOpenFile.de == NULL) {
			firstSpaceForOpenFile = i;
		}
	}

	// no spaces for open file
	if(firstSpaceForOpenFile == -1) {
		//kprintf("ERROR: no space for the file in open file table\n");
		return SYSERR;
	}

	oft[firstSpaceForOpenFile].state = FSTATE_OPEN;
	oft[firstSpaceForOpenFile].fileptr = 0;
	oft[firstSpaceForOpenFile].de = openFileDirectoryEntryPtr;
	oft[firstSpaceForOpenFile].in = *inodePtr;
	oft[firstSpaceForOpenFile].flag = O_RDWR;

	return OK;
}

int fs_unlink(char *filename) {
	int filenameIndex = -1;

	// iterate over all the files in the root directory
	for(int i = 0; i < DIRECTORY_SIZE; i++) {
		dirent_t currEntry = fsd.root_dir.entry[i];

		// if the file name matches then record the index and break
		// there shouldn't be duplicate names so don't need to check if inodeNum has be assigned yet
		if(strncmp(&(currEntry.name[0]), filename, FILENAMELEN) == 0) {
			filenameIndex = i;
			break;
		}
	}

	// there is no file with the name we are searching for
	if(filenameIndex == -1) {
		//kprintf("ERROR: A file does not exist with that name \n");
		return SYSERR;
	}

	int inodeNum = fsd.root_dir.entry[filenameIndex].inode_num;

	struct inode *inodePtr = (struct inode*) getmem(sizeof(inode_t));
	_fs_get_inode_by_num(dev0, inodeNum, inodePtr);

	if (inodePtr->nlink < 1) {
		//kprintf("ERROR: Inode has no links to it??? \n");
		return SYSERR;
	}

	if (inodePtr->nlink == 1) {
		// If the nlinks of the inode is just 1, then delete the respective inode along with its data blocks as well
		//kprintf("one nlink\n");

		// turn the inode's blocks to all zeros
		//memset(inodePtr->blocks, EMPTY, INODEBLOCKS);

		// write over old inode with blank one
		struct inode blankInode;
		blankInode.id = EMPTY;
		blankInode.type   = EMPTY;
		blankInode.nlink  = EMPTY;
		blankInode.device = dev0;
		blankInode.size   = EMPTY;
		memset(blankInode.blocks, EMPTY, INODEBLOCKS*sizeof(*blankInode.blocks));
		_fs_put_inode_by_num(dev0, inodeNum, &blankInode);

		fsd.inodes_used -= 1;
	}

	struct dirent *blankEntryPtr = (struct dirent*) getmem(sizeof(dirent_t));
	blankEntryPtr->inode_num = EMPTY;
	strcpy(&(blankEntryPtr->name[0]), "");

	fsd.root_dir.numentries += 1;
	fsd.root_dir.entry[filenameIndex] = *blankEntryPtr;

	int firstSpaceForOpenFile = -1;
	// check if the file is already open
	for(int i = 0; i < NUM_FD; i++) {
		filetable_t currOpenFile = oft[i];

		// assuming that all open files are at beginning and all empty spaces are at the end
		// remember the index of the first open spot
		if(filename == currOpenFile.de->name) {
			firstSpaceForOpenFile = i;
		}
	}

	// no spaces for open file
	if(firstSpaceForOpenFile == -1) {
		//kprintf("ERROR: no space for the file in open file table\n");
		return SYSERR;
	}

    oft[firstSpaceForOpenFile].state     = 0;
    oft[firstSpaceForOpenFile].fileptr   = 0;
    oft[firstSpaceForOpenFile].de        = NULL;
    oft[firstSpaceForOpenFile].in.id     = EMPTY;
    oft[firstSpaceForOpenFile].in.type   = 0;
    oft[firstSpaceForOpenFile].in.nlink  = 0;
    oft[firstSpaceForOpenFile].in.device = 0;
    oft[firstSpaceForOpenFile].in.size   = 0;
    memset(oft[firstSpaceForOpenFile].in.blocks, 0, INODEBLOCKS);
    oft[filenameIndex].flag      = 0;
	
	inodePtr->nlink -= 1;
	for (int i = 0; i < NUM_FD; i++) {
		if(oft[i].de->inode_num == inodeNum) {
			oft[i].in.nlink -= 1;
		}
  	}

  	return OK;
}

//#endif /* FS */