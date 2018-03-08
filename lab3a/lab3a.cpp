// NAME: Aidan Wolk, Christopher Aziz
// EMAIL: aidanwolk@g.ucla.edu, caziz@ucla.edu

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "ext2_fs.h"
#include <iomanip>
#include <ctime>
#include <cstring>
#include <cassert>
#include <vector>

using namespace std;

// number of bytes to store a block number
const int ptr_length = 4; //bytes

// populated based on file and supergroup
int image_fd;
unsigned int block_size;
unsigned int ptrs_per_block;

// Read from an offset from the open image file
// Handles error checking
void Pread(void *buf, size_t nbyte, off_t offset) {
	ssize_t res = pread(image_fd, buf, nbyte, offset);
	if (res < 0) {
		cerr << "Error (pread): " << strerror(errno) << endl;
		exit(2);
	} else if (static_cast<size_t>(res) < nbyte) {
		cerr << "Error (pread): expected more data" << endl;
		exit(2);
	}
}

// read a block from the open image file as an array of block numbers (for indirect blocks)
__u32* read_block_of_pointers(int block_num) {
	__u32 *ptrs = new __u32[ptrs_per_block];
	Pread(ptrs, block_size, block_num * block_size);
	return ptrs;
}

// read a block from the open image file as an array of characters
char* read_block(int block_num) {
	char* buff = new char[block_size];
	Pread(buff, block_size, block_num * block_size);
	return buff;
}

// signal a corrupt file and exit
void corruption() {
	cerr << "Error: Corruption detected" << endl;
	exit(2);
}

// reads the contents of a file inode
string read_file(const struct ext2_inode &inode) {
	string file = "";
	unsigned int num_blocks = inode.i_blocks / (block_size / 512); // number of file system blocks
	// Direct blocks
	unsigned int i; // number of content blocks read
	for (i = 0; i < 12; i++) {
		if (i == num_blocks)
			return file;
		if (inode.i_block[i] == 0)
			corruption();
		char *buff = read_block(inode.i_block[i]);
		file.append(buff, block_size);
		delete[] buff;
	}
	// Indirect block
	__u32 *ptrs = read_block_of_pointers(inode.i_block[12]);
	for (unsigned int j = 0; j < ptrs_per_block; j++, i++) {
		if (i == num_blocks)
			return file;
		__u32 ptr = ptrs[j];
		if (ptr == 0)
			corruption();
		char* buff = read_block(ptr);
		file.append(buff, block_size);
		delete[] buff;
	}
	delete[] ptrs;
	// Doubly indirect block
	__u32 *ptrs_l2 = read_block_of_pointers(inode.i_block[13]);
	for (unsigned int j = 0; j < ptrs_per_block; j++) {
		if (ptrs_l2[j] == 0)
			corruption();
		__u32 *ptrs = read_block_of_pointers(ptrs_l2[j]);
		for (unsigned int k = 0; k < ptrs_per_block; k++, i++) {
			if (i == num_blocks)
				return file;
			__u32 ptr = ptrs[k];
			if (ptr == 0)
				corruption();
			char* buff = read_block(ptr);
			file.append(buff, block_size);
			delete[] buff;
		}
		delete[] ptrs;
	}
	delete[] ptrs_l2;
	// Triply indirect block
	__u32 *ptrs_l3 = read_block_of_pointers(inode.i_block[14]);
	for (unsigned int j = 0; j < ptrs_per_block; j++) {
		if (ptrs_l3[j] == 0)
			corruption();
		__u32 *ptrs_l2 = read_block_of_pointers(ptrs_l3[j]);
		for (unsigned int k = 0; k < ptrs_per_block; k++) {
			if (ptrs_l2[k] == 0)
				corruption();
			__u32 *ptrs = read_block_of_pointers(ptrs_l2[k]);
			for (unsigned int m = 0; m < ptrs_per_block; m++, i++) {
				if (i == num_blocks)
					return file;
				__u32 ptr = ptrs[m];
				if (ptr == 0)
					corruption();
				char* buff = read_block(ptr);
				file.append(buff, block_size);
				delete[] buff;
			}
			delete[] ptrs;
		}
		delete[] ptrs_l2;
	}
	delete[] ptrs_l3;
	
	return file;
}

// recurse through indirect blocks and log information
void examine_indirect(int owner_inode_number, const struct ext2_inode &inode,
					  int level, int block_index, int logical_offset) {
	assert(1 <= level && level <= 3);
	if (block_index == 0)
		return;
	
	__u32 *ptrs = read_block_of_pointers(block_index);
	for (unsigned int j = 0; j < block_size / ptr_length; j++) {
		__u32 ptr = ptrs[j];
		if (ptr == 0)
			continue;
		cout << "INDIRECT,"
			<< owner_inode_number << "," // inode number of owning file
			<< level << "," // level of indirection
			<< logical_offset + j << "," // logical block offset
			<< block_index << "," // block number of block being scanned
			<< ptr << endl; // block number of referenced block
		if (level > 1)
			examine_indirect(owner_inode_number, inode, level - 1, ptr, logical_offset + j);
	}
	delete[] ptrs;
}

int main(int argc, const char * argv[]) {
	// Parse arguments
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <file.img>" << endl;
		return 1;
	}
	// Open input file
	image_fd = open(argv[1], O_RDONLY);
	if (image_fd < 0) {
		// Error opening image
		cerr << "Error: " << strerror(errno) << endl;
		return 1;
	}
	/* Superblock Summary */
	struct ext2_super_block superblock;
	Pread(&superblock, sizeof(superblock), 1024);
	block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
	ptrs_per_block = block_size / ptr_length; // for indirect blocks (to be used later)
	
	cout << "SUPERBLOCK,"
		<< superblock.s_blocks_count << "," // total number of blocks
		<< superblock.s_inodes_count << "," // total number of i-nodes
		<< block_size << "," // block size
		<< superblock.s_inode_size << "," // i-node size
		<< superblock.s_blocks_per_group << "," // blocks per group
		<< superblock.s_inodes_per_group << "," // inodes per group
		<< superblock.s_first_ino << endl; // first non-reserved inode
	/* Group Summary (assumes 1 group) */
	struct ext2_group_desc group;
	Pread(&group, sizeof(group), 1024 + sizeof(superblock));
	cout << "GROUP,"
		<< 0 << ","	// group number
		<< superblock.s_blocks_count << "," // blocks in this group, assumes 1 group
		<< superblock.s_inodes_count << "," // i-nodes in this group, assumes 1 group
		<< group.bg_free_blocks_count << "," // number of free blocks
		<< group.bg_free_inodes_count << "," // number of free i-nodes
		<< group.bg_block_bitmap << "," // block number of free block bitmap for this group
		<< group.bg_inode_bitmap << "," // block number of free i-node bitmap for this group
		<< group.bg_inode_table << endl; // block number of first block of i-nodes in this group
	/* Free Blocks */
	for (unsigned int block_num = 1; block_num <= superblock.s_blocks_count; block_num++) {
		int byte = (block_num - 1) >> 3;
		int bit = (block_num - 1) & 7;
		int address = group.bg_block_bitmap * block_size + byte;
		unsigned char byte_read;
		Pread(&byte_read, 1, address);
		bool is_free = ((byte_read >> bit) & 1) == 0;
		if (is_free)
			cout << "BFREE,"
				<< block_num << "\n";
	}
	/* Free I-Nodes */
	for (unsigned int inode_num = 1; inode_num <= superblock.s_inodes_count; inode_num++) {
		int byte = (inode_num - 1) >> 3;
		int bit = (inode_num - 1) & 7;
		int address = group.bg_inode_bitmap * block_size + byte;
		unsigned char byte_read;
		Pread(&byte_read, 1, address);
		bool is_free = ((byte_read >> bit) & 1) == 0;
		if (is_free)
			cout << "IFREE,"
				<< inode_num << "\n";
	}
	/* I-node summary */
	int first_inode_address =
		group.bg_inode_table * block_size;
	for (unsigned int i = 1; i <= superblock.s_inodes_per_group; i++) {
		struct ext2_inode inode;
		Pread(&inode, sizeof(inode), first_inode_address + (i - 1) * sizeof(struct ext2_inode));
		// skip unallocated inodes
		if (inode.i_mode == 0 || inode.i_links_count == 0)
			continue;
		
		char file_type = (inode.i_mode & 0x8000) ? 'f' :
		(inode.i_mode & 0x4000) ? 'd' :
		(inode.i_mode & 0xA000) ? 's' : '?';
		time_t ctime = (time_t) inode.i_ctime;
		time_t mtime = (time_t) inode.i_mtime;
		time_t atime = (time_t) inode.i_atime;
		const char *time_fmt = "%m/%d/%y %H:%M:%S";
		
		cout << "INODE,"
			<< i << "," // inode number
			<< file_type << "," // file type
			<< oct << (inode.i_mode & 4095) << "," // lower order 12 bits of mode
			<< dec << inode.i_uid << "," // owner
			<< inode.i_gid << "," // group
			<< inode.i_links_count << "," // link count
			<< put_time(gmtime(&ctime), time_fmt) << "," // creation time
			<< put_time(gmtime(&mtime), time_fmt) << "," // modification time
			<< put_time(gmtime(&atime), time_fmt) << "," // last access time
			<< inode.i_size << "," // size
			<< inode.i_blocks; // number of blocks
		for (int i = 0; i < 15; i++) // blocks
			cout << "," << inode.i_block[i];
		cout << endl;
		
		/* Process Indirection */
		examine_indirect(i, inode, 1, inode.i_block[12], 12); // indirect block
		examine_indirect(i, inode, 2, inode.i_block[13], 12 + ptrs_per_block); // double-indirect lock
		examine_indirect(i, inode, 3, inode.i_block[14],
						 12 + ptrs_per_block + ptrs_per_block * ptrs_per_block); // triple-indirect block
		
		/* Process Directory */
		if (file_type == 'd') {
			// read directory listing
			string file_contents = read_file(inode);
			const char *contents = file_contents.c_str();
			unsigned int offset = 0;
			while(offset < inode.i_size) {
				struct ext2_dir_entry entry;
				memcpy(&entry, contents + offset, sizeof(entry));
				if (entry.inode != 0)
					cout << "DIRENT,"
						<< i << ","	// inode number
						<< offset << "," // logical byte offset
						<< entry.inode << "," // inode number of referenced file
						<< entry.rec_len << "," // entry length
						<< (unsigned short) entry.name_len << "," // length of name
						<< "'" + string(entry.name, entry.name_len) + "'" << endl; // name
				// progress to next node in linked list
				offset += entry.rec_len;
			}
		}
	}
	return 0;
}
