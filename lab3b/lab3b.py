#!/usr/bin/env python3
# NAME: Aidan Wolk, Christopher Aziz
# EMAIL: aidanwolk@g.ucla.edu, caziz@ucla.edu
# Lab3B analyzes a CSV file to report file system inconsistencies and corruptions
import sys
from collections import namedtuple, defaultdict, Counter

# File system data structures
Superblock = namedtuple('Superblock', ['num_blocks', 'num_inodes', 'block_size', 'inode_size',
                                       'blocks_per_group', 'inodes_per_group', 'first_inode'])
Group = namedtuple('Group', ['num', 'num_blocks', 'num_inodes', 'num_free_blocks', 'num_free_inodes',
                             'free_block_bitmap_ptr', 'free_inode_bitmap_ptr', 'first_block_inode_ptr'])
INode = namedtuple('INode', ['num', 'type', 'mode', 'owner', 'group', 'link_count',
                             'ctime', 'mtime', 'atime', 'size', 'num_blocks',
                             'direct_blocks', 'indirect_l1_ptr', 'indirect_l2_ptr', 'indirect_l3_ptr'])
DirEntry = namedtuple('DirEntry', ['parent_inode', 'logical_offset', 'inode', 'entry_length', 'name_length', 'name'])
IndirectReference = namedtuple('IndirectReference', ['inode', 'level', 'logical_offset',
                                                     'parent_block_num', 'referenced_block_num'])


# Handle CSV Input
def read_csv(csv_filename):
    """Parse an input CSV file"""
    try:
        with open(csv_filename) as csv_file:
            for line in csv_file:
                entries = line[:-1].split(',')  # split line into comma separated items (removing ending newline)
                parse_line(entries)
    except IOError:
        print('Error: Unable to read file "{}"'.format(csv_filename), file=sys.stderr)
        sys.exit(1)


def parse_line(entries):
    """Parse a line from CSV"""
    global superblock, group, free_blocks, free_inodes, inodes, dirents, indirects

    if entries[0] == 'SUPERBLOCK':
        superblock = Superblock(*[int(i) for i in entries[1:]])
    elif entries[0] == 'GROUP':
        group = Group(*[int(i) for i in entries[1:]])
        reserved_blocks.add(group.free_block_bitmap_ptr)
        reserved_blocks.add(group.free_inode_bitmap_ptr)
        # Assume inode table starts at 5
        for inode_table_block in range(5, 5 + int(group.num_inodes / (superblock.block_size / superblock.inode_size))):
            reserved_blocks.add(inode_table_block)
    elif entries[0] == 'BFREE':
        free_blocks.add(int(entries[1]))
    elif entries[0] == 'IFREE':
        free_inodes.add(int(entries[1]))
    elif entries[0] == 'INODE':
        inodes.append(INode(int(entries[1]), entries[2], int(entries[3], 8),
                            int(entries[4]), int(entries[5]), int(entries[6]),
                            entries[7], entries[8], entries[9], int(entries[10]), int(entries[11]),
                            [int(i) for i in entries[12:24]], *[int(i) for i in entries[24:]]))
    elif entries[0] == 'DIRENT':
        dirents.append(DirEntry(*[int(i) for i in entries[1:6]], entries[6]))
    elif entries[0] == 'INDIRECT':
        indirects.append(IndirectReference(*[int(i) for i in entries[1:]]))


# Results from parsing
superblock = None  # should only be 1
group = None  # should only be 1
free_blocks = set()
free_inodes = set()
inodes = []
dirents = []
indirects = []

# Magic values
reserved_blocks = {1, 2}
reserved_inodes = set(range(11))  # first 11 inodes are reserved
block_num_size_bytes = 4  # number of bytes to hold a block number in an indirect block

invalid_filesystem = False


def print_inc(*args, **kwargs):
    global invalid_filesystem
    invalid_filesystem = True
    print(*args, **kwargs)


# Perform all 3 audits
def audit():
    ptrs_per_block = superblock.block_size // block_num_size_bytes
    indirection_level = {1: 'INDIRECT', 2: 'DOUBLE INDIRECT', 3: 'TRIPLE INDIRECT'}

    inode_links = Counter()
    inode_dirents = defaultdict(list)
    references = defaultdict(list)
    allocated_inodes = set()

    parent_dirent_inode = defaultdict(list)
    parent_dirent_inode[2] = 2  # '..' in root (2) is root
    dotdot_dirents = []

    # Scan all directory entries
    for dirent in dirents:
        inode_links[dirent.inode] += 1
        if dirent.name == "'.'":
            if dirent.inode != dirent.parent_inode:
                print_inc('DIRECTORY INODE', dirent.parent_inode, 'NAME', dirent.name,
                          'LINK TO INODE', dirent.inode, 'SHOULD BE', dirent.parent_inode)
        elif dirent.name == "'..'":
            dotdot_dirents.append(dirent)
        else:
            parent_dirent_inode[dirent.inode] = dirent.parent_inode

        if dirent.inode < 1 or dirent.inode > group.num_inodes:
            print_inc('DIRECTORY INODE', dirent.parent_inode, 'NAME', dirent.name, 'INVALID INODE', dirent.inode)
        else:
            inode_dirents[dirent.inode].append(dirent)

    # Check correctness of '..' files
    for dotdot_dirent in dotdot_dirents:
        container = dotdot_dirent.parent_inode
        parent = parent_dirent_inode[container]
        if parent != dotdot_dirent.inode:
            print_inc('DIRECTORY INODE', container, 'NAME', dotdot_dirent.name,
                      'LINK TO INODE', dotdot_dirent.inode, 'SHOULD BE', parent)

    # Scan all inodes
    for inode in inodes:
        allocated_inodes.add(inode.num)

        # Check if inode link count matches actual link count
        if inode.link_count != inode_links[inode.num]:
            print_inc('INODE', inode.num, 'HAS', inode_links[inode.num], 'LINKS BUT LINKCOUNT IS', inode.link_count)

        # Check if inode is on free list
        if inode.num in free_inodes:
            print_inc('ALLOCATED INODE', inode.num, 'ON FREELIST')

        # Check direct blocks in inode
        for offset, direct_block_num in enumerate(inode.direct_blocks):
            if direct_block_num == 0:  # ignore 0 entry in block list
                continue
            reference = 'BLOCK {} IN INODE {} AT OFFSET {}'.format(direct_block_num, inode.num, offset)
            references[direct_block_num].append(reference)
            if direct_block_num < 0 or direct_block_num >= superblock.num_blocks:
                print_inc('INVALID', reference)
            if direct_block_num in reserved_blocks:
                print_inc('RESERVED', reference)
            if direct_block_num in free_blocks:
                print_inc('ALLOCATED BLOCK', direct_block_num, 'ON FREELIST')

        # Check indirect blocks in inode
        logical_offset = {1: 12, 2: 12 + ptrs_per_block, 3: 12 + ptrs_per_block + ptrs_per_block ** 2}
        for level, block_num in enumerate([inode.indirect_l1_ptr, inode.indirect_l2_ptr, inode.indirect_l3_ptr]):
            level += 1  # account for 0-based indexing
            if block_num == 0:
                continue
            reference = '{} BLOCK {} IN INODE {} AT OFFSET {}'.format(
                indirection_level[level], block_num, inode.num, logical_offset[level])
            references[block_num].append(reference)
            if block_num < 0 or block_num >= superblock.num_blocks:
                print_inc('INVALID', reference)
            if block_num in reserved_blocks:
                print_inc('RESERVED', reference)
            if block_num in free_blocks:
                print_inc('ALLOCATED BLOCK', block_num, 'ON FREELIST')

    # Scan all indirect entries
    for indirect in indirects:
        reference = '{} BLOCK {} IN INODE {} AT OFFSET {}'.format(
            indirection_level[indirect.level], indirect.referenced_block_num, indirect.inode, indirect.logical_offset)
        references[indirect.referenced_block_num].append(reference)
        if indirect.referenced_block_num < 0 or indirect.referenced_block_num >= superblock.num_blocks:
            print_inc('INVALID', reference)
        if indirect.referenced_block_num in reserved_blocks:
            print_inc('RESERVED', reference)
        if indirect.referenced_block_num in free_blocks:
            print_inc('ALLOCATED BLOCK', indirect.referenced_block_num, 'ON FREELIST')

    # Check for duplicate block references
    for block_num, allocations in references.items():
        if len(allocations) > 1:
            for allocation in allocations:
                print_inc('DUPLICATE', allocation)

    # Check for unreferenced blocks that are not in the free list
    for block_num in set(range(superblock.first_inode, superblock.num_blocks)) - free_blocks - set(references.keys()):
        print_inc('UNREFERENCED BLOCK', block_num)

    # Check for unallocated inodes that are not in the free list
    for inode_num in set(range(group.num_inodes)) - free_inodes - allocated_inodes - reserved_inodes:
        print_inc('UNALLOCATED INODE', inode_num, 'NOT ON FREELIST')

    # Check for unallocated directory entries
    for inode_num in set(inode_dirents.keys()) - allocated_inodes:
        for dirent in inode_dirents[inode_num]:
            print_inc('DIRECTORY INODE', dirent.parent_inode, 'NAME', dirent.name, 'UNALLOCATED INODE', inode_num)


# Argument parsing
if len(sys.argv) != 2:
    print('Usage: {} <file.csv>'.format(sys.argv[0]), file=sys.stderr)
    sys.exit(1)
read_csv(sys.argv[1])
audit()

# Check for inconsistency
if invalid_filesystem:
    sys.exit(2)
