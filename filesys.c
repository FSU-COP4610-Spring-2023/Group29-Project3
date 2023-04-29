// starter file provided by operating systems ta's

// include libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUFSIZE 40

#define BYTES_PER_DIR_ENTRY 32  // number of bytes in a directory entry
#define PATH_SIZE 4096          // max possible size for path string.
#define OPEN_FILE_TABLE_SIZE 10 // max files for files.
#define MAX_NAME_LENGTH 11      // 11 in total but 3 for extensions, we only use 8.

FILE *fp;
FILE *fp1;
FILE *fp2;

// data structures for FAT32
// Hint: BPB, DIR Entry, Open File Table -- how will you structure it?

typedef struct __attribute__((packed))
{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytesPerSec;
    unsigned char BPB_SecsPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short BPB_RootEntCnt;
    unsigned short BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short BPB_FATSz16;
    unsigned short BPB_SecPerTrk;
    unsigned short BPB_NumHeads;
    unsigned int BPB_HiddSec;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned short BPB_ExtFlags;
    unsigned short BPB_FSVer;
    unsigned int BPB_RootClus;
    unsigned short BPB_FSInfo;
    unsigned short BPB_BkBootSe;
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int BS_VollD;
    unsigned char BS_VolLab[11];
    unsigned char BS_FilSysType[8];
    unsigned char empty[420];
    unsigned short Signature_word;
} BPB;

typedef struct __attribute__((packed))
{
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned short DIR_CrtTime;
    unsigned short DIR_CrtDate;
    unsigned short DIR_LastAccDate;
    unsigned short DIR_FstClusHi;
    unsigned short DIR_WrtTime;
    unsigned short DIR_WrtDate;
    unsigned short DIR_FstClusLo;
    unsigned int DIR_FileSize;
} DirEntry;

// stack implementaiton -- you will have to implement a dynamic stack
// Hint: For removal of files/directories

typedef struct
{
    char path[PATH_SIZE];
    DirEntry directoryEntry;
    unsigned int first_cluster;        // First cluster location
    unsigned int first_cluster_offset; // Offset of first cluster in bytes
    unsigned int offset;
    int mode; //  0 = free
} opened_file;

typedef struct
{
    char path[PATH_SIZE]; // path string
    // add a variable to help keep track of current working directory in file.
    unsigned long byte_offset;
    unsigned long root_offset;
    unsigned int cluster;
    // Hint: In the image, where does the first directory entry start?
} CWD;

typedef struct DirEntryNode
{
    DirEntry entry;
    struct DirEntryNode *next;
} DirEntryNode;

typedef struct
{
    int size;
    char **items;
} tokenlist;

// function declarations
tokenlist *tokenize(char *input);
void free_tokens(tokenlist *tokens);
char *get_input();
void add_token(tokenlist *tokens, char *item);
void add_to_path(char *dir);
void info();                                     // done
void cd(char *DIRNAME);                          // done
void ls(void);                                   // done
void mkdir(char *DIRNAME);                       // half
void creat(char *FILENAME);                      // done
void cp(char *FILENAME, char *TO);               // half
void open(char *FILENAME, int FLAGS);            // done
void close(char *FILENAME);                      // done
void lsof(void);                                 // done
void size(char *FILENAME);                       // done
void lseek(char *FILENAME, unsigned int OFFSET); // have to start
void read(char *FILENAME, unsigned int size);    // have to start
void write(char *FILENAME, char *STRING);        // have to start
void rm(char *FILENAME);                         // have to start
void rmdir(char *DIRNAME);                       // have to start
// void rename_file(char *FILENAME, char *NEW_FILENAME); // have to start

// global variables
CWD cwd;
FILE *fp; // file pointers
BPB bpb;  // boot sector information
DirEntry current_entry;
opened_file files_opened[10];

// These variables should be all we need for accessing, modifying, and working with the FAT32 files
unsigned long fat_begin_lba;
unsigned long cluster_begin_lba;
unsigned char sectors_per_cluster;
unsigned long root_dir_first_cluster;
unsigned long root_dir_clusters;
unsigned long first_data_sector;
unsigned long first_data_sector_offset;
unsigned long Partition_LBA_Begin;
unsigned long currentDirectory;
int number_files_open = 0;

// Partition_LBA_Begin can be found using the Microsoft docs and is essential to finding proper locations
int main(int argc, char *argv[])
{
    // error checking for number of arguments.
    if (argc != 2)
    {
        printf("invalid number of arguments.\n");
        return -1; // return error indicator if number of args is not 2.
    }
    // read and open argv[1] in file pointer.
    fp = fopen(argv[1], "r+");
    if (fp == NULL)
    { // r+ stands for read,write,append.
        printf("%s does not exist\n.", argv[1]);
        return -1;
    }

    // obtain important information from bpb as well as initialize any important global variables
    fread(&bpb, sizeof(BPB), 1, fp);
    sectors_per_cluster = bpb.BPB_SecsPerClus;
    root_dir_first_cluster = bpb.BPB_RootClus;

    cluster_begin_lba = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);

    // where Partition_LBA_Begin is the logical block address for the start of the partition
    // bpb holds the boot sector information
    // RsvdSecCnt is the # of reserved sectors at the start of the partition (boot sector too)
    fat_begin_lba = bpb.BPB_RsvdSecCnt;
    Partition_LBA_Begin = fat_begin_lba + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);

    // sum of the partition start sector and the number of reserved sectors
    // NumFats is # of FAT tables and FATSz32 is the size of the tables in 'sectors'
    cluster_begin_lba = Partition_LBA_Begin + bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);

    root_dir_clusters = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytesPerSec - 1)) / bpb.BPB_BytesPerSec;
    first_data_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + root_dir_clusters;
    first_data_sector_offset = first_data_sector * bpb.BPB_BytesPerSec;
    cwd.root_offset = first_data_sector_offset;
    cwd.byte_offset = cwd.root_offset;
    cwd.cluster = bpb.BPB_RootClus;

    memset(cwd.path, 0, PATH_SIZE);
    strcat(cwd.path, argv[1]);
    // parser
    char *input;
    while (1)
    {
        printf("%s/> ", cwd.path);
        input = get_input();
        tokenlist *tokens = tokenize(input);
        if (strcmp(tokens->items[0], "info") == 0)
        {
            info();
        }
        else if (strcmp(tokens->items[0], "exit") == 0)
        {
            fclose(fp); // closes the image and deallocates it from memory
            return 0;
        }
        else if (strcmp(tokens->items[0], "cd") == 0)
        {
            if (tokens->items[1] != NULL)
                cd(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "ls") == 0)
        {
            ls();
        }
        else if (strcmp(tokens->items[0], "mkdir") == 0)
        {
            mkdir(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "creat") == 0)
        {
            creat(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "cp") == 0)
        {
            cp(tokens->items[1], tokens->items[2]);
        }
        else if (strcmp(tokens->items[0], "open") == 0)
        {
            if (tokens->items[2] != NULL && tokens->items[1] != NULL)
            {
                int flags;
                if (strcmp(tokens->items[2], "-r") == 0)
                {
                    flags = 1;
                }
                if (strcmp(tokens->items[2], "-w") == 0)
                {
                    flags = 2;
                }
                if (strcmp(tokens->items[2], "-rw") == 0)
                {
                    flags = 3;
                }
                if (strcmp(tokens->items[2], "-wr") == 0)
                {
                    flags = 4;
                }
                open(tokens->items[1], flags);
            }
            else
                printf("Missing file arguments (-r|-w|-rw|-wr)\n");
        }
        else if (strcmp(tokens->items[0], "close") == 0)
        {
            close(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "lsof") == 0)
        {
            lsof();
        }
        else if (strcmp(tokens->items[0], "size") == 0)
        {
            size(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "lseek") == 0)
        {
            lseek(tokens->items[1], atoi(tokens->items[2]));
        }
        else if (strcmp(tokens->items[0], "read") == 0)
        {
            read(tokens->items[1], tokens->items[2]);
        }
        else if (strcmp(tokens->items[0], "write") == 0)
        {
            write(tokens->items[1], tokens->items[2]);
        }
        /*else if (strcmp(tokens->items[0], "rename") == 0)
        {
            rename_file(tokens->items[1], tokens->items[2]);
        }*/
        else if (strcmp(tokens->items[0], "rm") == 0)
        {
            rm(tokens->items[1]);
        }
        else if (strcmp(tokens->items[0], "rmdir") == 0)
        {
            rmdir(tokens->items[1]);
        }
        // add_to_path(tokens->items[0]); // move this out to its correct place;
        free(input);
        free_tokens(tokens);
    }

    return 0;
}

// helper functions -- to navigate file image

/* This function does basically the same operations as the ls function
but instead of printing out all of the information it checks each of
the entries/clusters to see if they are valid or not, this function
is essential to the project (some sections were generated by Copilot)*/
int locate_directory(char *DIRNAME)
{
    int i, j;
    unsigned long originalPos = ftell(fp);
    unsigned int currCluster = cwd.cluster;
    fseek(fp, cwd.byte_offset, SEEK_SET);
    // while currentCluster < total number of sectors in partition
    while (currCluster < bpb.BPB_TotSec32)
    {
        // move to position of first byte in the cluster
        fseek(fp, (first_data_sector + ((currCluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
        for (i = 0; i < 16; i++)
        {
            // retrieve information about current entry
            fread(&current_entry, sizeof(DirEntry), 1, fp);
            // deleted/does not exist/long file name
            if (current_entry.DIR_Attr == 0x0F || current_entry.DIR_Name == 0x00)
                continue;
            for (j = 0; j < 11; j++)
            {
                if (current_entry.DIR_Name[j] == 0x20)
                    current_entry.DIR_Name[j] = 0x00;
            }
            if (strcmp(current_entry.DIR_Name, DIRNAME) == 0)
            {
                if (current_entry.DIR_Attr == 0x10)
                {
                    // move pointer back to beginning potision
                    fseek(fp, originalPos, SEEK_SET);
                    return 0;
                }
                else
                {
                    // move pointer back to beginning potision
                    fseek(fp, originalPos, SEEK_SET);
                    return -1;
                }
            }
        }
        fseek(fp, (bpb.BPB_RsvdSecCnt * bpb.BPB_BytesPerSec + (currCluster * 4)), SEEK_SET);
        fread(&currCluster, sizeof(int), 1, fp);
    }
    fseek(fp, originalPos, SEEK_SET);
    return 1;
}
// converts the LBA address we get from fread() and turns it into an int for offset

int find_first_available_cluster()
{
    int cluster = -1;
    int first_cluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz32;
    int num_clusters = bpb.BPB_TotSec32 - first_cluster;
    // Iterate through the FAT entries until we find an unallocated cluster
    for (int i = first_cluster; i < first_cluster + num_clusters; i++)
    {
        unsigned int fat_entry = get_fat_entry(i);
        if (fat_entry == 0)
        {
            cluster = i;
            break;
        }
    }
    return cluster;
}

unsigned int get_current_cluster()
{
    unsigned int cluster = (current_entry.DIR_FstClusLo) | ((current_entry.DIR_FstClusHi) << 16);
    if (cluster == 0)
    {
        return root_dir_first_cluster;
    }
    return cluster;
}

unsigned int allocate_cluster()
{
    unsigned int fat_entry;
    unsigned int cluster = find_first_available_cluster();
    if (cluster == -1)
    {
        printf("No free cluster available\n");
        return -1;
    }
    
    fat_entry = get_fat_entry(cluster);
    if (fat_entry != 0x00000000 && fat_entry != 0x0FFFFFFF)
    {
        printf("Cluster already allocated\n");
        return -1;
    }
    set_fat_entry(cluster, 0x0FFFFFFF);
    return cluster;
}

void set_fat_entry(unsigned int cluster, unsigned int value)
{
    // Calculate the offset of the FAT entry in bytes
    unsigned int offset = cluster * 4;

    // Calculate the sector containing the FAT entry
    unsigned int sector = bpb.BPB_RsvdSecCnt + (offset / bpb.BPB_BytesPerSec);

    // Calculate the byte offset within the sector
    unsigned int byte_offset = offset % bpb.BPB_BytesPerSec;

    // Seek to the sector containing the FAT entry
    fseek(fp, sector * bpb.BPB_BytesPerSec, SEEK_SET);

    // Read the sector into a buffer
    unsigned char sector_buffer[bpb.BPB_BytesPerSec];
    fread(sector_buffer, 1, bpb.BPB_BytesPerSec, fp);

    // Update the value of the FAT entry in the buffer
    *((unsigned int *)(sector_buffer + byte_offset)) = value;

    // Seek back to the sector and write the updated buffer
    fseek(fp, sector * bpb.BPB_BytesPerSec, SEEK_SET);
    fwrite(sector_buffer, 1, bpb.BPB_BytesPerSec, fp);
}

int get_fat_entry(unsigned int cluster_idx)
{
    unsigned int fat_offset = cluster_idx * 4;
    unsigned int fat_sector = bpb.BPB_RsvdSecCnt + (fat_offset / bpb.BPB_BytesPerSec);
    unsigned int fat_entry_offset = fat_offset % bpb.BPB_BytesPerSec;

    fseek(fp, fat_sector * bpb.BPB_BytesPerSec + fat_entry_offset, SEEK_SET);

    unsigned int fat_entry_value;
    fread(&fat_entry_value, sizeof(fat_entry_value), 1, fp);

    return fat_entry_value;
}


// commands -- all commands mentioned in part 2-6 (17 cmds)

// Mount

/* This function basically just takes in the file struct
bpb as initialized earlier and then prints out all of the info
based off of the mounted image. The image is opened and read
by using fread() and fopen() in main.*/
void info()
{
    int clusters = (bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + root_dir_clusters) / bpb.BPB_SecsPerClus);
    printf("Position of root: %d\n", bpb.BPB_RootClus);
    printf("Bytes per sector: %d\n", bpb.BPB_BytesPerSec);
    printf("Sectors per cluster: %d\n", bpb.BPB_SecsPerClus);
    printf("# of clusters in data region: %d\n", clusters);
    printf("# of entries in one fat: %d\n", ((bpb.BPB_FATSz32 * bpb.BPB_BytesPerSec) / 4));
    printf("Size of image (in bytes): %d\n", bpb.BPB_TotSec32 * bpb.BPB_BytesPerSec);
}

// Navigation

// cd need some error management on handling going backwards through directories
void cd(char *DIRNAME)
{
    int exists = locate_directory(DIRNAME);
    if (exists == 0)
    {
        unsigned int cluster = get_current_cluster();
        if (cluster == 0)
        {
            cwd.byte_offset = first_data_sector_offset;
            cwd.cluster = bpb.BPB_RootClus;
        }
        else
        {
            cwd.byte_offset = (first_data_sector + ((cluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec;
            cwd.cluster = cluster;
        }

        if (strcmp(DIRNAME, "..") == 0)
        {
            // Go up one directory
            char *last_slash = strrchr(cwd.path, '/');
            if (last_slash != NULL)
            {
                *last_slash = '\0';
            }
        }
        else if (strcmp(DIRNAME, ".") != 0)
        {
            // Go into the specified directory
            if (strcmp(cwd.path, "/") == 0)
            {
                // If we're at the root directory, don't add a leading slash
                sprintf(cwd.path, "%s", DIRNAME);
            }
            else
            {
                sprintf(cwd.path, "%s/%s", cwd.path, DIRNAME);
            }
        }
    }
    else if (exists == -1)
    {
        printf("%s is a file not a directory\n", DIRNAME);
    }
    else if (exists == 1)
    {
        printf("this directory does not exist.\n");
    }
}

/* ls iterates through each directory and cluster to get the
directory names and print them out. This is mainly done by
taking the main pointer and starting at the beginning
of the root directory. Once there we iterate through
for the # of sectors and move to the first byte of
each cluster to find directory names for each cluster.
We then move everything back in place so that we can
use our offsets again for another function. */

void ls(void)
{
    DirEntryNode *head = NULL;
    DirEntryNode *tail = NULL;
    unsigned long originalPosition = ftell(fp);
    // Move file pointer to the beginning of the root directory
    fseek(fp, (((root_dir_first_cluster - 2) * bpb.BPB_SecsPerClus) + cluster_begin_lba), SEEK_SET);
    unsigned int currentCluster = cwd.cluster;
    // while currentCluster < total number of sectors in partition
    unsigned long numSectors = bpb.BPB_TotSec32;
    for (numSectors; currentCluster < numSectors; numSectors++)
    {
        // move to the first byte of the cluster
        fseek(fp, (first_data_sector + ((currentCluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
        for (int i = 0; i < 16; i++)
        {
            // retrieve information about current entry
            DirEntryNode *node = (DirEntryNode *)malloc(sizeof(DirEntryNode));
            fread(&(node->entry), sizeof(DirEntry), 1, fp);
            // deleted/does not exist/long file name
            if (node->entry.DIR_Attr == 0x0F || node->entry.DIR_Name[0] == 0x00)
                continue;
            for (int j = 0; j < 11; j++)
            {
                if (node->entry.DIR_Name[j] == 0x20)
                    node->entry.DIR_Name[j] = 0x00;
            }
            // add node to linked list
            node->next = NULL;
            if (tail == NULL)
            {
                head = node;
                tail = node;
            }
            else
            {
                tail->next = node;
                tail = node;
            }
        }
        // move to fat entry offset
        fseek(fp, ((bpb.BPB_RsvdSecCnt * bpb.BPB_BytesPerSec) + (currentCluster * 4)), SEEK_SET);
        fread(&currentCluster, sizeof(int), 1, fp);
    }
    // print directory entries from linked list
    DirEntryNode *currentNode = head;
    while (currentNode != NULL)
    {
        printf("%s ", currentNode->entry.DIR_Name);
        currentNode = currentNode->next;
    }
    printf("\n");
    // free linked list memory
    currentNode = head;
    while (currentNode != NULL)
    {
        DirEntryNode *nextNode = currentNode->next;
        free(currentNode);
        currentNode = nextNode;
    }
    fseek(fp, originalPosition, SEEK_SET);
}

// Create
/*
mkdir creates a new directory entry and uses allocate_cluster() to allocate a new cluster.
allocate_cluster() updates FAT and new directory entries "." and ".." are created.
Currently, "." does not function properly.
*/
void mkdir(char *DIRNAME)
{
    if (locate_directory(DIRNAME) != 1)
    {
        printf("Directory already exists\n");
        return;
    }
    else
    {
        int i;
        unsigned long originalPosition = ftell(fp);
        // Create the file entry
        DirEntry new_entry;
        // Initialize the entry with zeroes
        memset(&new_entry, 0, sizeof(DirEntry));
        // Copy the filename into the entry
        memcpy(new_entry.DIR_Name, DIRNAME, strlen(DIRNAME));
        // 0x10 means directory instead of a file
        new_entry.DIR_Attr = 0x10;
        // Set the first cluster to the first free cluster
        unsigned int cluster = allocate_cluster();
        if(cluster == -1)
        {
            printf("Failed to allocate cluster\n");
            return;
        }
        new_entry.DIR_FstClusLo = cluster;
        // Set the file size to 0
        new_entry.DIR_FileSize = 0x0;
        unsigned long currentCluster = cwd.cluster;
        unsigned long numSectors = bpb.BPB_TotSec32;
        // here we use the bread and butter of directory navigation
        for (numSectors; currentCluster < numSectors; numSectors++)
        {
            // move to the first byte of the cluster
            fseek(fp, (first_data_sector + ((currentCluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
            for (i = 0; i < (bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / 32); i++)
            {
                fread(&current_entry, sizeof(DirEntry), 1, fp);
                if (current_entry.DIR_Name[0] == 0x00)
                {
                    fseek(fp, -sizeof(DirEntry), SEEK_CUR);
                    fwrite(&new_entry, sizeof(DirEntry), 1, fp);
                    fseek(fp, originalPosition, SEEK_SET);

                    // Create "." entry
                    DirEntry dot_entry;
                    memset(&dot_entry, 0, sizeof(DirEntry));
                    memcpy(dot_entry.DIR_Name, ".", 1);
                    dot_entry.DIR_Attr = 0x10;
                    dot_entry.DIR_FstClusLo = new_entry.DIR_FstClusLo;
                    dot_entry.DIR_FileSize = 0x0;
                    fseek(fp, (first_data_sector + ((new_entry.DIR_FstClusLo - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
                    fwrite(&dot_entry, sizeof(DirEntry), 1, fp);
                    
                    // Create ".." entry
                    DirEntry dotdot_entry;
                    memset(&dotdot_entry, 0, sizeof(DirEntry));
                    memcpy(dotdot_entry.DIR_Name, "..", 2);
                    dotdot_entry.DIR_Attr = 0x10;
                    dotdot_entry.DIR_FstClusLo = cwd.cluster;
                    dotdot_entry.DIR_FileSize = 0x0;
                    fseek(fp, (first_data_sector + ((new_entry.DIR_FstClusLo - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
                    fwrite(&dotdot_entry, sizeof(DirEntry), 1, fp);
                }
            }
            printf("directory created successfully\n");
            return;
        }
    }
}

/* this uses the same search method as locate_directory() and ls()
but pushes a new directory entry instead of showing what is already there*/
void creat(char *FILENAME)
{
    // Check if a file with the same name already exists
    if (locate_directory(FILENAME) == 1)
    {
        int i;
        unsigned long originalPosition = ftell(fp);
        unsigned long currentCluster = cwd.cluster;
        unsigned long numSectors = bpb.BPB_TotSec32;
        DirEntry new_entry = {0};
        memcpy(new_entry.DIR_Name, FILENAME, strlen(FILENAME));
        new_entry.DIR_Attr = 0x20;
        new_entry.DIR_FstClusLo = 0x0;
        new_entry.DIR_FileSize = 0x0;

        // Search for the first available directory entry
        for (numSectors; currentCluster < numSectors; numSectors++)
        {
            fseek(fp, (first_data_sector + ((currentCluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
            for (i = 0; i < (bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / 32); i++)
            {
                fread(&current_entry, sizeof(DirEntry), 1, fp);
                if (current_entry.DIR_Name[0] == 0x00)
                {
                    fseek(fp, -sizeof(DirEntry), SEEK_CUR);
                    fwrite(&new_entry, sizeof(DirEntry), 1, fp);
                    fseek(fp, originalPosition, SEEK_SET);
                    printf("File created successfully\n");
                    return;
                }
            }
        }
    }
    else
    {
        printf("File already exists\n");
    }
}
void cp(char *FILENAME, char *TO)
{
    // Check if the file exists
    int file_cluster = locate_directory(FILENAME);
    if (file_cluster == -1)
    {
        printf("File not found\n");
        return;
    }

    // If 'to' is a directory, copy the file directly into the folder
    int dir_cluster = locate_directory(TO);
    if (dir_cluster != -1)
    {
        // Copy the file into the directory
        creat(FILENAME);
        return;
    }

    // If 'to' is not a valid entry, create a copy of the file within the current working directory
    // and assign it the name given by 'to'.
    char *new_filename = TO;
    int dot_index = strcspn(TO, ".");
    if (dot_index > 0)
    {
        // If 'to' has a file extension, create a copy with the new name and extension
        new_filename = malloc(sizeof(char) * strlen(TO));
        char *name = strtok(TO, ".");
        char *extension = strtok(NULL, ".");
        sprintf(new_filename, "%s_copy.%s", name, extension);
    }
    else
    {
        // If 'to' has no file extension, create a copy with the new name and the original extension
        char *extension = strrchr(FILENAME, '.');
        if (extension == NULL)
        {
            // If the original file has no extension, assign it '.copy'
            extension = ".copy";
        }
        sprintf(new_filename, "%s%s", TO, extension);
    }
}

// Read
void open(char *filename, int flags)
{
    // Check if the file is already opened
    locate_directory(filename);
    bool is_already_opened = false;
    for (int i = 0; i < OPEN_FILE_TABLE_SIZE; i++)
    {
        if (files_opened[i].mode != 0 && strcmp(files_opened[i].path, filename) == 0)
        {
            printf("File is already opened\n");
            is_already_opened = true;
            break;
        }
    }
    if (is_already_opened)
    {
        printf("file is already open");
        return;
    }

    // Check if the max # of opened files is reached
    if (number_files_open == OPEN_FILE_TABLE_SIZE)
    {
        printf("you have the max files opened\n");
        return;
    }

    // Find an empty file slot
    int empty_slot_index = -1;
    for (int i = 0; i < OPEN_FILE_TABLE_SIZE; i++)
    {
        if (files_opened[i].mode == 0)
        {
            empty_slot_index = i;
            break;
        }
    }
    if (empty_slot_index == -1)
    {
        printf("Unexpected error: no empty file slot found\n");
        return;
    }

    // Open the file
    unsigned short high = current_entry.DIR_FstClusHi;
    unsigned short low = current_entry.DIR_FstClusLo;
    unsigned int cluster = (high << 8) | low;
    unsigned long cluster_offset = (first_data_sector + ((cluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec;
    if (cluster == 0)
    { // Edge case of ".."
        cluster = bpb.BPB_RootClus;
        cluster_offset = cwd.root_offset;
    }
    strcpy(files_opened[empty_slot_index].path, cwd.path);
    files_opened[empty_slot_index].offset = 0;
    files_opened[empty_slot_index].directoryEntry = current_entry;
    files_opened[empty_slot_index].first_cluster = cluster;
    files_opened[empty_slot_index].first_cluster_offset = cluster_offset;
    files_opened[empty_slot_index].mode = flags;
    number_files_open++;
}

void close(char *FILENAME)
{
    int index = -1;
    for (int i = 0; i < 10; i++)
    {
        if (files_opened[i].mode != 0 && strcmp(files_opened[i].directoryEntry.DIR_Name, FILENAME) == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("File %s is not open\n", FILENAME);
        return;
    }

    files_opened[index].mode = 0;
    number_files_open--;
    printf("File %s closed\n", FILENAME);
}

void lsof(void)
{
    printf("index, file name, mode, offset, path \n");
    for (int i = 0; i < number_files_open; i++)
    {
        if (files_opened[i].mode == 0)
        {
            continue;
        }
        char *mode;
        switch (files_opened[i].mode)
        {
        case 1:
            mode = "-r";
            break;
        case 2:
            mode = "-w";
            break;
        case 3:
            mode = "-rw";
            break;
        case 4:
            mode = "-wr";
            break;
        default:
            mode = "unknown";
            break;
        }

        printf("%d, %s, %s, %d, %s\n", i, files_opened[i].directoryEntry.DIR_Name, mode, files_opened[i].offset, files_opened[i].path);
    }
}

/* Just prints out the current entry's file size
after checking if the file is a directory or a
file and if it exists or not using locate_directory() */
void size(char *FILENAME)
{
    int i = locate_directory(FILENAME);

    if (i == 1 || i == 0)
    {
        printf("%s\n", "is directory or does not exist");
        return;
    }
    printf("%s %d %s\n", "File is", current_entry.DIR_FileSize, "bytes");
}

void lseek(char *FILENAME, unsigned int OFFSET)
{
    // Find the file in the list of opened files
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(files_opened[i].directoryEntry.DIR_Name, FILENAME) == 0)
        {
            // Update the file offset
            files_opened[i].offset = OFFSET;
            printf("File offset for %s set to %d bytes\n", FILENAME, OFFSET);
            return;
        }
    }
    printf("File %s not found or not opened\n", FILENAME);
}
void read(char *FILENAME, unsigned int size)
{
    // make sure the file is readable
    if (current_entry.DIR_Attr > 0x04 && current_entry.DIR_Attr < 0x01)
    {
        printf("%s\n", "Permission denied, file non-readable");
        return;
    }
    // Find the file in the list of opened files
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(files_opened[i].directoryEntry.DIR_Name, FILENAME) == 0)
        {
            unsigned int offsetInitalValue = files_opened[i].offset;
            unsigned int bytesRead = 0;
            printf("File contents:\n");
            while (bytesRead < size && files_opened[i].offset < files_opened[i].directoryEntry.DIR_FileSize)
            {
                char buffer[512];
                fseek(fp, files_opened[i].directoryEntry.DIR_FstClusLo * bpb.BPB_BytesPerSec + files_opened[i].offset, SEEK_SET);
                int bytesToRead = (size - bytesRead < 512) ? size - bytesRead : 512;
                int bytesReadThisIter = fread(buffer, 1, bytesToRead, fp);
                fwrite(buffer, 1, bytesReadThisIter, stdout);
                bytesRead += bytesReadThisIter;
                files_opened[i].offset += bytesReadThisIter;
            }
            return;
        }
    }
    // (
    //         if (strcmp(files_opened[i].directoryEntry.DIR_Name, FILENAME) == 0)
    //         {
    //             //if size + offset >= size of file
    //             if(files_opened[i].offset + size >= files_opened[i].directoryEntry.DIR_FileSize)
    //             {
    //                 while(files_opened[i].offset < size + files_opened[i].offset)
    //                     {
    //                     printf();
    //                     }
    //                 lseek(FILENAME, files_opened[i].offset);
    //                 return;
    //             }
    //             //if it is not greater
    //             else
    //             {
    //                 while(files_opened[i].offset < size + files_opened[i].offset)
    //                     {
    //                     printf();
    //                     }
    //                 lseek(FILENAME, files_opened[i].offset);
    //                 return;
    //             }

    //         })
}

// Update
void write(char *FILENAME, char *STRING)
{
    // make sure the file is readable
    if (current_entry.DIR_Attr > 0x04 && current_entry.DIR_Attr < 0x02)
    {
        printf("%s\n", "Permission denied, file non-writable.");
        return;
    }
    // Find the file in the list of opened files
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(files_opened[i].directoryEntry.DIR_Name, FILENAME) == 0)
        {
            int initOff = files_opened[i].offset;
        }
    }
    printf("%s\n", "Error, file not found.");
    return;
}

void rename_(char *FILENAME, char *NEWFILENAME)
{
    // Search for the directory entry corresponding to FILENAME
    DirEntry new_entry;
    locate_directory(FILENAME);
    if (locate_directory(FILENAME) == -1)
    {
        printf("Error: %s does not exist\n", FILENAME);
        return;
    }
    else if (new_entry.DIR_Attr & 0x10)
    {
        printf("Error: %s is a directory, not a file\n", FILENAME);
        return;
    }

// Delete
void rm(char *FILENAME)
{
    void remove_file(char *FILENAME)
    {
        // Find the directory entry for the given file
        int i, j;
        int found = 0;
        DirEntry entry;
        int cluster = cwd.cluster;
        while (cluster != 0xFFFFFFFF && cluster != 0x0FFFFFF8 && cluster != 0x0FFFFFFE && cluster != 0xFFFFFF0F && cluster != 0xFFFFFFF)
        {
            long byteOffsetOfCluster = (first_data_sector + ((cluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec;
            fseek(fp, byteOffsetOfCluster, SEEK_SET);
            for (i = 0; i < 16; i++)
            {
                fread(&entry, sizeof(DirEntry), 1, fp);
                if (entry.DIR_Attr == 0x0F || entry.DIR_Name[0] == 0x00)
                {
                    continue;
                }
                for (j = 0; j < 11; j++)
                {
                    if (entry.DIR_Name[j] == 0x20)
                    {
                        entry.DIR_Name[j] = 0x00;
                    }
                }
                if (strcmp(FILENAME, entry.DIR_Name) == 0)
                {
                    found = 1;
                    break;
                }
            }
            if (found)
            {
                break;
            }
            cluster = get_next_cluster(cluster);
        }

        // If the file was not found, print an error message and return
        if (!found)
        {
            printf("Error: File not found\n");
            return;
        }

        // If the file is a directory, print an error message and return
        if (entry.DIR_Attr & 0x10)
        {
            printf("Error: '%s' is a directory, use rmdir command instead\n", FILENAME);
            return;
        }

        // Remove the file by clearing the FAT entries and updating the directory entry
        unsigned int clusterNumber = entry.DIR_FstClusLo + (entry.DIR_FstClusHi * 65536);
        clear_FAT(clusterNumber);
        entry.DIR_Name[0] = 0x00;
        long byteOffsetOfEntry = ftell(fp) - sizeof(DirEntry);
        fseek(fp, byteOffsetOfEntry, SEEK_SET);
        fwrite(&entry, sizeof(DirEntry), 1, fp);
    }
}
void rmdir(char *DIRNAME)
{
    // Check if the directory exists
    unsigned long dir_cluster = locate_directory(DIRNAME);
    if (dir_cluster != 0)
    {
        printf("Directory not found\n");
        return;
    }

    // Deallocate the clusters used by the directory
    unsigned long offset = first_data_sector * bpb.BPB_BytesPerSec + dir_cluster * sizeof(DirEntry);
    fseek(fp, offset, SEEK_SET);
    DirEntry deleted_entry;
    memset(&deleted_entry, 0, sizeof(DirEntry));
    fwrite(&deleted_entry, sizeof(DirEntry), 1, fp);
    // free cluster chain
    unsigned int cluster = cwd.cluster;
    unsigned int prev_cluster = cluster;
    while (cluster < 0x0FFFFFF8)
    {
        prev_cluster = cluster;
        cluster = get_fat_entry(cluster);
        set_fat_entry(prev_cluster, 0x00000000);
    }
    set_fat_entry(prev_cluster, 0x00000000);
    printf("Directory removed successfully\n");

}


// add directory string to cwd path -- helps keep track of where we are in image.
void add_to_path(char *dir)
{
    if (dir == NULL)
    {
        return;
    }
    else if (strcmp(dir, "..") == 0)
    {
        char *last = strrchr(cwd.path, '/');
        if (last != NULL)
        {
            *last = '\0';
        }
    }
    else if (strcmp(dir, ".") != 0)
    {
        strcat(cwd.path, "/");
        strcat(cwd.path, dir);
    }
}

void free_tokens(tokenlist *tokens)
{
    for (int i = 0; i < tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}

// take care of delimiters {'\"', ' '}
tokenlist *tokenize(char *input)
{
    int is_in_string = 0;
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    tokens->size = 0;

    tokens->items = (char **)malloc(sizeof(char *));
    char **temp;
    int resizes = 1;
    char *token = input;

    for (; *input != '\0'; input++)
    {
        if (*input == '\"' && !is_in_string)
        {
            is_in_string = 1;
            token = input + 1;
        }
        else if (*input == '\"' && is_in_string)
        {
            *input = '\0';
            add_token(tokens, token);
            while (*(input + 1) == ' ')
            {
                input++;
            }
            token = input + 1;
            is_in_string = 0;
        }
        else if (*input == ' ' && !is_in_string)
        {
            *input = '\0';
            while (*(input + 1) == ' ')
            {
                input++;
            }
            add_token(tokens, token);
            token = input + 1;
        }
    }
    if (is_in_string)
    {
        printf("error: string not properly enclosed.\n");
        tokens->size = -1;
        return tokens;
    }

    // add in last token before null character.
    if (*token != '\0')
    {
        add_token(tokens, token);
    }

    return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
    int i = tokens->size;
    tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *)malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);
    tokens->size += 1;
}

char *get_input()
{
    char *buf = (char *)malloc(sizeof(char) * BUFSIZE);
    memset(buf, 0, BUFSIZE);
    char c;
    int len = 0;
    int resizes = 1;

    int is_leading_space = 1;

    while ((c = fgetc(stdin)) != '\n' && !feof(stdin))
    {

        // remove leading spaces.
        if (c != ' ')
        {
            is_leading_space = 0;
        }
        else if (is_leading_space)
        {
            continue;
        }

        buf[len] = c;

        if (++len >= (BUFSIZE * resizes))
        {
            buf = (char *)realloc(buf, (BUFSIZE * ++resizes) + 1);
            memset(buf + (BUFSIZE * (resizes - 1)), 0, BUFSIZE);
        }
    }
    buf[len + 1] = '\0';

    // remove trailing spaces.
    char *end = &buf[len - 1];

    while (*end == ' ')
    {
        *end = '\0';
        end--;
    }

    return buf;
}