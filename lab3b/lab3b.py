#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

import sys

def block_pointer_handler(block_num, inode_num, block_type, offset, start_num, end_num):
    #Invalid block
    if block_num < 0 or block_num > end_num - 1:
        sys.stdout.write('INVALID ' + block_type + ' ' + str(block_num) + ' IN INODE ' + str(inode_num) + ' AT OFFSET ' + str(offset) + '\n')
        return False

    #Reserved block
    elif block_num > 0 and block_num < start_num:
        sys.stdout.write('RESERVED ' + block_type + ' ' + str(block_num) + ' IN INODE ' + str(inode_num) + ' AT OFFSET ' + str(offset) + '\n')
        return False

    return True


def duplicate_handler(block_num, inode_num, block_type):
    offset = 0
    if block_type == 'INDIRECT BLOCK':
        offset = 12
    if block_type == 'DOUBLE INDIRECT BLOCK':
        offset = 12 + 256
    if block_type == 'TRIPPLE INDIRECT BLOCK':
        offset = 12 + 256 + (256 * 256)

    sys.stdout.write('DUPLICATE ' + block_type + ' ' + str(block_num) + ' IN INODE ' + str(inode_num) + ' AT OFFSET ' + str(offset) + '\n')


def block_consistency_audit(file):
    block_map = {}

    #Parse through each line, collect data from summary file, compare data with block pointers
    for line in file:
        summary_data = line.split(',')
        #SUPERBLOCK
        if summary_data[0] == 'SUPERBLOCK':
            inode_size = int(summary_data[4])
            block_size = int(summary_data[3])

        #GROUP
        if summary_data[0] == 'GROUP':
            num_blocks = int(summary_data[2])
            num_inodes = int(summary_data[3])
            inode_logical_offset = int(summary_data[8].split('\n')[0])
            free_block_start = inode_logical_offset + (inode_size * num_inodes / block_size)

        #BFREE
        if summary_data[0] == 'BFREE':
            block_num = int(summary_data[1].split('\n')[0])
            block_map[block_num] = 'FREE'

        #INODE - Check validity of block pointers
        if summary_data[0] == 'INODE':
            inode_num = summary_data[1]

            #Iterate through block addresses and compare data. Print error message if invalid, store valid data
            for num in range(12, 27):
                block_num = int(summary_data[num].split('\n')[0])
                if block_num != 0 and num < 24 and block_num not in block_map:
                    if block_pointer_handler(block_num, inode_num, 'BLOCK', 0, free_block_start, num_blocks):
                        block_map[block_num] = (inode_num, 'BLOCK')

                elif block_num != 0 and num == 24 and block_num not in block_map:
                    if block_pointer_handler(block_num, inode_num, 'INDIRECT BLOCK', 12, free_block_start, num_blocks):
                        block_map[block_num] = (inode_num, 'INDIRECT BLOCK')

                elif block_num != 0 and num == 25 and block_num not in block_map:
                    if block_pointer_handler(block_num, inode_num, 'DOUBLE INDIRECT BLOCK', 268, free_block_start, num_blocks):
                        block_map[block_num] = (inode_num, 'DOUBLE INDIRECT BLOCK')

                elif block_num != 0 and num == 26 and block_num not in block_map:
                    if block_pointer_handler(block_num, inode_num, 'TRIPPLE INDIRECT BLOCK', 65804, free_block_start, num_blocks):
                        block_map[block_num] = (inode_num, 'TRIPPLE INDIRECT BLOCK')

                elif block_num != 0 and block_map[block_num] == 'FREE':
                    sys.stdout.write('ALLOCATED BLOCK ' + str(block_num) + ' ON FREELIST\n')

                elif block_num != 0 and block_map[block_num] != 'FREE':
                    block_type = 'BLOCK'
                    if num == 24: block_type = 'INDIRECT BLOCK';                        
                    if num == 25: block_type = 'DOUBLE INDIRECT BLOCK';
                    if num == 26: block_type = 'TRIPPLE INDIRECT BLOCK';
                    
                    duplicate_handler(block_num, inode_num, block_type)
                    duplicate_handler(block_num, block_map[block_num][0], block_map[block_num][1])

        #INDIRECT
        if summary_data[0] == 'INDIRECT':
            inode_num = int(summary_data[1])
            indirect_level = int(summary_data[2])
            logical_offset = int(summary_data[3])
            block_num = int(summary_data[5].split('\n')[0])

            block_type = 'BLOCK'
            if indirect_level == 1: block_type = 'INDIRECT BLOCK';
            if indirect_level == 2: block_type = 'DOUBLE INDIRECT BLOCK';
            if indirect_level == 3: block_type = 'TRIPPLE INDIRECT BLOCK';

            #Check for blocks that are allocated to some file but still appear on free list
            if block_num in block_map:
                if block_map[block_num] == 'FREE':
                    sys.stdout.write('ALLOCATED BLOCK ' + str(block_num) + ' ON FREELIST\n')
                    continue

            #Check for duplicate blocks
            if block_num in block_map:
                if block_map[block_num] != 'FREE':
                    duplicate_handler(block_num, inode_num, block_type)
                    duplicate_handler(block_num, block_map[block_num][0], block_map[block_num][1])
                    continue

            #Check for invalid/reserved blocks        
            if (block_pointer_handler(block_num, inode_num, block_type, logical_offset, free_block_start, num_blocks)):
                block_map[block_num] = (inode_num, block_type)

    #Check for unreferenced blocks (do not appear in any file or on free list)
    for num in range(free_block_start, num_blocks):
        if num not in block_map:
            sys.stdout.write('UNREFERENCED BLOCK ' + str(num) + '\n')
            
    file.seek(0)


def inode_allocation_audit(file):
    inode_map = {}

    #Parse through each line, collect data from summary file, check inode allocation consistency
    for line in file:
        summary_data = line.split(',')
        inode_num = int(summary_data[1])

        if summary_data[0] == 'SUPERBLOCK':
            num_inodes = int(summary_data[2])
            free_inode_start = int(summary_data[7].split('\n')[0])

        elif summary_data[0] == 'IFREE':
            inode_map[inode_num] = 'FREE'

        elif summary_data[0] == 'INODE' and inode_num not in inode_map:
            inode_map[inode_num] = 'ALLOCATED'

        elif summary_data[0] == 'INODE':
            sys.stdout.write('ALLOCATED INODE ' + str(inode_num) + ' ON FREELIST\n')
            inode_map[inode_num] = 'ALLOCATED'

    #Check for unallocated inodes (unallocated and not on free i-node list)
    for num in range(free_inode_start, num_inodes + 1):
        if num not in inode_map:
            sys.stdout.write('UNALLOCATED INODE ' + str(num) + ' NOT ON FREELIST\n')

    file.seek(0)
    return inode_map


def directory_consistency_audit(file, inode_map):
    free_inodes = inode_map    
    unfree_inodes = []

    #Initlialize maps for link count and path data checks
    link_map = {} 
    real_link_count = {}
    immediate_path = {} 

    for key in free_inodes:
        if free_inodes[key] != 'FREE':
            unfree_inodes.append(key)

    for key in unfree_inodes:
        del free_inodes[key]

    #Collect INODE data
    for line in file:
        summary_data = line.split(',')
        if summary_data[0] == 'INODE':
            inode_num = int(summary_data[1])
            link_count = int(summary_data[6])
            real_link_count[inode_num] = 0
            link_map[inode_num] = link_count

    file.seek(0)

    #Parse through and audit DIRENT lines
    for line in file:
        summary_data = line.split(',')

        #Scan directory entries and check validity and allocation status of referenced inodes
        if summary_data[0] == 'DIRENT':
            parent_inode_num = int(summary_data[1])
            file_inode_num = int(summary_data[3])
            dir_name = summary_data[6].split('\n')[0]

            #Unallocated inode
            if file_inode_num in free_inodes:
                sys.stdout.write('DIRECTORY INODE ' + str(parent_inode_num) + ' NAME ' + dir_name + ' UNALLOCATED INODE ' + str(file_inode_num) + '\n')

            #Invalid inode 
            elif file_inode_num not in free_inodes and file_inode_num not in link_map:
                sys.stdout.write('DIRECTORY INODE ' + str(parent_inode_num) + ' NAME ' + dir_name + ' INVALID INODE ' + str(file_inode_num) + '\n')

            #Valid inode, increment number of links discovered, update parent directory data
            else:
                real_link_count[file_inode_num] += 1
                if file_inode_num not in immediate_path:
                    immediate_path[file_inode_num] = parent_inode_num

        #Scan directory and check validity of (.) and (..) links
        if summary_data[0] == 'DIRENT':
            parent_inode_num = int(summary_data[1])
            file_inode_num = int(summary_data[3])
            dir_name = summary_data[6].split('\n')[0]

            if (dir_name == "'.'" and (file_inode_num != parent_inode_num)):
                sys.stdout.write('DIRECTORY INODE ' + str(parent_inode_num) + ' NAME ' + dir_name + ' LINK TO INODE ' + str(file_inode_num) + ' SHOULD BE ' + str(parent_inode_num) + '\n')

            elif (dir_name == "'..'"):
                deez_inode_number = immediate_path[parent_inode_num]
                if(file_inode_num != deez_inode_number):
                    sys.stdout.write('DIRECTORY INODE ' + str(parent_inode_num) + ' NAME ' + dir_name + ' LINK TO INODE ' + str(file_inode_num) + ' SHOULD BE ' + str(deez_inode_number) + '\n')

    #Check reference count of inodes against real_link_count
    for inode_num in link_map:
        if real_link_count[inode_num] != link_map[inode_num]:
            sys.stdout.write('INODE ' + str(inode_num) + ' HAS ' + str(real_link_count[inode_num]) + ' LINKS BUT LINKCOUNT IS ' + str(link_map[inode_num]) + '\n')

    file.seek(0)


def main():
    #Check arguments
    if len(sys.argv) == 1:
        sys.stderr.write('ERROR: Did not detect summary file arguments\n')
        exit(1)

    #Open fs summary files, run audits. Skip argv[0].
    for argument in sys.argv:
        if argument == 'lab3b.py': 
            continue

        try:
            file = open(argument)
            block_consistency_audit(file)
            inode_map = inode_allocation_audit(file)
            directory_consistency_audit(file, inode_map)
            file.close()

        except IOError as e:
            sys.stderr.write('ERROR: Could not open fs summary file\n')
            exit(1)

if __name__ == '__main__':
    main()


