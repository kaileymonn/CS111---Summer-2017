//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "ext2_fs.h"


//----------------GLOBAL VARIABLES----------------//


struct ext2_inode local_inode;
struct ext2_super_block superblock_total;
struct ext2_group_desc* group_total;
struct ext2_dir_entry directory;

struct group_sum {
    int blockCount;
    int inodeCount;
    int freeblockCount;
    int freeinodeCount;
    int freeblockNum;
    int freeinodeNum;
    int blockinodeNum;
};

char* file_name;
int image_fd;
int output_fd;
int group_number;
int num_dir_inodes = 0;

uint32_t i32;
uint16_t i16;
uint8_t i8;

const int SUPERBLOCK_OFFSET = 1024;
const int BLOCKSIZE = 1024;



//--------------ADDITIONAL FUNCTIONS--------------//

void print_directory_entries(struct ext2_dir_entry directory, int parent_inode_num, int logical_byte_offset);

void print_indirect_refs(struct ext2_dir_entry directory, int parent_inode_num, int logical_block_offset, int indirect, int block_num, int i);

void inode_summary();

void format_time(uint32_t time_stamp, char* buffer);


//------------------MAIN ROUTINE------------------//

int main(int argc, char **argv) {
    if(argc != 2) {
      perror("ERROR: Invalid arguments");
      exit(1);
    }

    else {
      int len = strlen(argv[1]) + 1;
      file_name = malloc(len);
      file_name = argv[1];
    }

    image_fd = open(file_name, O_RDONLY);

    if(image_fd == -1) {
      perror("ERROR: Could not open image file");
      exit(1);
    }

    output_fd = creat("summary.csv", S_IRWXU);
    
    //SUPERBLOCK SUMMARY
    dprintf(output_fd, "SUPERBLOCK,");
    
    pread(image_fd, &superblock_total, sizeof(struct ext2_super_block), SUPERBLOCK_OFFSET);
    //2. total number of blocks
    dprintf(output_fd, "%d,", superblock_total.s_blocks_count);
    
    //3. total number of i-nodes
    dprintf(output_fd, "%d,", superblock_total.s_inodes_count);
    
    //4. block size
    dprintf(output_fd, "%d,", 1024 << superblock_total.s_log_block_size);
    
    //5. i-node size
    dprintf(output_fd, "%d,", superblock_total.s_inode_size);
    
    //6. blocks per group
    dprintf(output_fd, "%d,", superblock_total.s_blocks_per_group);
    
    //7. i-nodes per group
    dprintf(output_fd, "%d,", superblock_total.s_inodes_per_group);
    
    //8. first non-reserved i-node
    dprintf(output_fd, "%d\n", superblock_total.s_first_ino);
    

    //GROUP SUMMARY
    int remaining_blocks = superblock_total.s_blocks_count % superblock_total.s_blocks_per_group;
    int remaining_inodes = superblock_total.s_inodes_count % superblock_total.s_inodes_per_group;
    int STARTOFFSET = SUPERBLOCK_OFFSET + BLOCKSIZE;
    group_number = superblock_total.s_blocks_count / superblock_total.s_blocks_per_group + 1; //number of groups
    group_total = malloc(group_number * sizeof(struct ext2_group_desc));
        
    for(int i = 0; i < group_number; i++) {
      //2. group number
      dprintf(output_fd, "GROUP,%d,", i);
      
      //3. total number of blocks in this group
      if(i != group_number - 1 || remaining_blocks == 0) {dprintf(output_fd, "%d,", superblock_total.s_blocks_per_group);}
      else {dprintf(output_fd, "%d,", remaining_blocks);}
      
      //4. total number of inodes in this group
      if(i != group_number - 1 || remaining_inodes == 0) {dprintf(output_fd, "%d,", superblock_total.s_inodes_per_group);}
      else {dprintf(output_fd, "%d,", remaining_inodes);}
      
      pread(image_fd, &group_total[i], sizeof(struct ext2_group_desc), STARTOFFSET + i*sizeof(struct ext2_group_desc));
      
      //5. number of free blocks
      dprintf(output_fd, "%d,", group_total[i].bg_free_blocks_count);
      
      //6. number of free i-nodes
      dprintf(output_fd, "%d,", group_total[i].bg_free_inodes_count);
      
      //7. block number of free block bitmap for this group
      dprintf(output_fd, "%d,", group_total[i].bg_block_bitmap);
      
      //8. block number of free i-node bitmap for this group
      dprintf(output_fd, "%d,", group_total[i].bg_inode_bitmap);
    
      //9. block number of first block of i-nodes in this group
      dprintf(output_fd, "%d\n", group_total[i].bg_inode_table);
    }
    
    //FREE BLOCK ENTRIES
    int num_blocks = (1024 << superblock_total.s_log_block_size);
    for(int i = 0; i < group_number; i++) {
      for(int j = 0; j < num_blocks; j++) {
	pread(image_fd, &i8, 1, group_total[i].bg_block_bitmap*num_blocks + j);
	
	for(int k = 0; k < 8; k++) {
	  if((i8 & (1 << k)) == 0) {dprintf(output_fd, "BFREE,%d\n", (i*superblock_total.s_blocks_per_group) + (j*8) + (k+1));}
	}
      }
    }

    //FREE I-NODE ENTRIES
    for(int i = 0; i < group_number; i++) {
      for(int j = 0; j < num_blocks; j++) {
	pread(image_fd, &i8, 1, group_total[i].bg_inode_bitmap*num_blocks + j);
	
	for(int k = 0; k < 8; k++) {
	  if((i8 & (1 << k)) == 0) {dprintf(output_fd, "IFREE,%d\n", (i*superblock_total.s_inodes_per_group) + (j*8) + (k+1));}
	}
      }
    }

    //INODE SUMMARY, DIRECTORY ENTRIES, INDIRECT BLOCK REFERENCES
    inode_summary();

    close(output_fd);

    //Write to STDOUT for checking
    int fd = open("./summary.csv", O_RDONLY);
    int len;
    char buf[1024];
    while((len = read(fd, buf, 1024)) > 0) {write(1, buf, len);}
    
    close(fd);
}



//------------FUNCTION IMPLEMENTATIONS------------//

void print_directory_entries(struct ext2_dir_entry directory, int parent_inode_num, int logical_byte_offset) {
  if(directory.inode == 0) {return;}
  dprintf(output_fd, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_inode_num, logical_byte_offset, directory.inode, directory.rec_len, directory.name_len, directory.name);
}

void print_indirect_refs(struct ext2_dir_entry directory, int parent_inode_num, int logical_block_offset, int indirect, int block_num, int i) {
  if(directory.inode == 0) {return;}
  dprintf(output_fd, "INDIRECT,%d,%d,%d,%d,%d\n", parent_inode_num, indirect, logical_block_offset, block_num, i);
}

void inode_summary() {
  char cTimeString[20], mTimeString[20], aTimeString[20];
  int itable_offset = SUPERBLOCK_OFFSET + 4*BLOCKSIZE; //inode table start offset

  //Iterate through groups
  for(int k = 0; k < group_number; k++) {
    //Iterate through inodes
    for(int m = 0; m < superblock_total.s_inodes_count; m++) {
      pread(image_fd, &local_inode, sizeof(struct ext2_inode), itable_offset + m * sizeof(struct ext2_inode));
      if(local_inode.i_mode != 0 && local_inode.i_links_count != 0) {
	//2. inode number
	dprintf(output_fd, "INODE,%d,", m + 1);
	
	//3. file type
	char fileType = '?';
	if((local_inode.i_mode >> 12) == 0x8) 
	  fileType = 'f';

	if((local_inode.i_mode >> 12) == 0xA) 
	  fileType = 's';

	if((local_inode.i_mode >> 12) == 0x4) {
	  fileType = 'd';
	  num_dir_inodes++;
	}
	dprintf(output_fd, "%c,", fileType);
	
	//4-7. mode, owner, group, link count
	dprintf(output_fd, "%o,%d,%d,%d,", local_inode.i_mode & 0xFFF, local_inode.i_uid, local_inode.i_gid, local_inode.i_links_count);
	
	//8-10. time of last inode change, modification time, time of last access
	format_time(local_inode.i_ctime, cTimeString);
	format_time(local_inode.i_mtime, mTimeString);
	format_time(local_inode.i_atime, aTimeString);
	dprintf(output_fd, "%s,%s,%s,", cTimeString, mTimeString, aTimeString);
	
	//11-12. file size, number of blocks
	dprintf(output_fd, "%d,%d", local_inode.i_size, local_inode.i_blocks);
	
	//15 block addresses
	if(fileType != 's') 
	  for(int j = 0; j < EXT2_N_BLOCKS; j++) {dprintf(output_fd, ",%d", local_inode.i_block[j]);}
	else
	  dprintf(output_fd, ",%d", local_inode.i_block[0]); //if symbolic link, print only first direct block address
	
	dprintf(output_fd, "\n");
	
	//DIRECTORY ENTRIES
	if(fileType == 'd') {
	  for(int j = 0; j < EXT2_NDIR_BLOCKS; j++) {
	    if(local_inode.i_block[j] == 0) {break;} 	    
	    int directory_offset = local_inode.i_block[j] * BLOCKSIZE;
	    int current_offset = 0;
	    
	    while(current_offset < BLOCKSIZE) {
	      pread(image_fd, &directory, sizeof(struct ext2_dir_entry), directory_offset + current_offset);
	      print_directory_entries(directory, m + 1, current_offset);
	      current_offset += directory.rec_len;
	    }
	  }
	}
	
	//INDIRECT BLOCK REFERENCES
	if(fileType == 'f') {
	  int* level_one = malloc(BLOCKSIZE);
	  int* level_two = malloc(BLOCKSIZE);
	  int* level_three = malloc(BLOCKSIZE);
	  
	  //SINGLE INDIRECT 
	  if(local_inode.i_block[12] > 0) {
	    pread(image_fd, level_one, BLOCKSIZE, (local_inode.i_block[12] * BLOCKSIZE));
	    
	    for(int i = 0; i < BLOCKSIZE / 4; i++) {
	      if(level_one[i] != 0) { 
		int directory_offset = level_one[i] * BLOCKSIZE;
		int current_offset = 0;
	
		while(current_offset < BLOCKSIZE) {
		  pread(image_fd, &directory, sizeof(struct ext2_dir_entry), directory_offset + current_offset);
		  print_indirect_refs(directory, m + 1, 12 + i, 1, local_inode.i_block[12], level_one[i]);
		  current_offset += directory.rec_len;
		}
	      }
	    }
	  }
	  
	  //DOUBLE INDIRECT 
	  if(local_inode.i_block[13] > 0) {
	    pread(image_fd, level_one, BLOCKSIZE, local_inode.i_block[13] * BLOCKSIZE);
	    
	    for(int a = 0; a < BLOCKSIZE / 4; a++) {
	      pread(image_fd, level_two, BLOCKSIZE, level_one[a] * BLOCKSIZE);
	      
	      for(int b = 0; b < BLOCKSIZE / 4; b++) {
		if(level_two[b] != 0) {  
		  int directory_offset = level_two[b] * BLOCKSIZE;
		  int current_offset = 0;
	
		  while(current_offset < BLOCKSIZE) {
		    pread(image_fd, &directory, sizeof(struct ext2_dir_entry), directory_offset + current_offset);
		    //printf("a: %d, b: %d\n", a, b);
		    print_indirect_refs(directory, m + 1, 256-(a+1)+13+a, 2, local_inode.i_block[13], level_one[a]);
		    print_indirect_refs(directory, m + 1, 256-(a+1)+13+a+b, 1, level_one[a], level_two[b]);
		    current_offset += directory.rec_len;
		  }
		}
	      }
	    }
	  }
	  
	  //TRIPLE INDIRECT 
	  if(local_inode.i_block[14] > 0) {
	    pread(image_fd, level_one, BLOCKSIZE, local_inode.i_block[14] * BLOCKSIZE);
	    
	    for(int a = 0; a < BLOCKSIZE / 4; a++) {
	      pread(image_fd, level_two, BLOCKSIZE, level_one[a] * BLOCKSIZE);
	      
	      for(int b = 0; b < BLOCKSIZE / 4; b++) {
		pread(image_fd, level_three, BLOCKSIZE, level_two[b] * BLOCKSIZE);
		
		for(int c = 0; c < BLOCKSIZE / 4; c++) {
		  if(level_three[c] != 0) {
		    int directory_offset = level_three[c] * BLOCKSIZE;
		    int current_offset = 0;
		    
		    while(current_offset < BLOCKSIZE) {
		      pread(image_fd, &directory, sizeof(struct ext2_dir_entry), directory_offset + current_offset);
		      //printf("a: %d, b: %d, c: %d\n", a, b, c);
		      print_indirect_refs(directory, m + 1, (256-a)*(256-b)+(256-c)+14+a, 3, local_inode.i_block[14], level_one[a]);
		      print_indirect_refs(directory, m + 1, (256-a)*(256-b)+(256-c)+14+a+b, 2, level_one[a], level_two[b]);
		      print_indirect_refs(directory, m + 1, (256-a)*(256-b)+(256-c)+14+a+b+c, 1, level_two[b], level_three[c]);
		      current_offset += directory.rec_len;
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
}

void format_time(uint32_t time_stamp, char* buf) {
  time_t epoch = time_stamp;
  struct tm ts = *gmtime(&epoch);
  strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}
