// starter file provided by operating systems ta's

// include libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char path[PATH_SIZE]; // path string
    // add a variable to help keep track of current working directory in file.
    unsigned long byteOffset;
    unsigned long rootOffset;
    unsigned int cluster;
    // Hint: In the image, where does the first directory entry start?
} CWD;

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
void info();
void cd(char *DIRNAME);
void ls(void);
void mkdir(char *DIRNAME);
void creat(char *FILENAME);
void cp(char *FILENAME, unsigned int TO);
void open(char *FILENAME, int FLAGS);
void close(char *FILENAME);
void lsof(void);
void size(char *FILENAME);
void lseek(char *FILENAME, unsigned int OFFSET);
void read(char *FILENAME, unsigned int size);
void write(char *FILENAME, char *STRING);
// void rename_file(char *FILENAME, char *NEW_FILENAME);
void rm(char *FILENAME);
void rmdir(char *DIRNAME);

// global variables
CWD cwd;
FILE *fp; // file pointers
BPB bpb;  // boot sector information
DirEntry currentEntry;

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
    cwd.rootOffset = first_data_sector_offset;
    cwd.byteOffset = cwd.rootOffset;
    cwd.cluster = bpb.BPB_RootClus;

    memset(cwd.path, 0, PATH_SIZE);

    // parser
    char *input;
    while (1)
    {
        printf("%s", argv[1]);
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
            open(tokens->items[1], tokens->items[2]);
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
            lseek(tokens->items[1], tokens->items[2]);
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
int locateDirectory(char *DIRNAME)
{
    int i, j;
    unsigned long originalPos = ftell(fp);
    unsigned int currCluster = cwd.cluster;
    fseek(fp, cwd.byteOffset, SEEK_SET);
    // while currentCluster < total number of sectors in partition
    while (currCluster < bpb.BPB_TotSec32)
    {
        // move to position of first byte in the cluster
        fseek(fp, (first_data_sector + ((currCluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
        for (i = 0; i < 16; i++)
        {
            // retrieve information about current entry
            fread(&currentEntry, sizeof(DirEntry), 1, fp);
            // deleted/does not exist/long file name
            if (currentEntry.DIR_Attr == 0x0F || currentEntry.DIR_Name == 0x00)
                continue;
            for (j = 0; j < 11; j++)
            {
                if (currentEntry.DIR_Name[j] == 0x20)
                    currentEntry.DIR_Name[j] = 0x00;
            }
            if (strcmp(currentEntry.DIR_Name, DIRNAME) == 0)
            {
                if (currentEntry.DIR_Attr == 0x10)
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
int LBAToOffset(unsigned int sector)
{
    if (sector == 0)
        sector = 2;
    return ((sector - 2) * bpb.BPB_BytesPerSec) + (bpb.BPB_BytesPerSec * bpb.BPB_RsvdSecCnt) + (bpb.BPB_NumFATs * bpb.BPB_FATSz32 * bpb.BPB_BytesPerSec);
}

int find_first_available_cluster()
{
    int cluster = -1;
    int first_cluster = cwd.cluster;
    int num_clusters = bpb.BPB_TotSec32;
    // Iterate through the FAT entries until we find an unallocated cluster
    /*for (int i = first_cluster; i < first_cluster + num_clusters; i++)
    {
        unsigned int fat_entry = (bpb.BPB_RsvdSecCnt * bpb.BPB_BytesPerSec + (cwd.cluster * 4));
        if (fat_entry == 0)
        {
            cluster = i;
            return cluster;
        }
    }*/
    for (num_clusters; first_cluster < num_clusters; num_clusters++)
    {
        // move to the first byte of the cluster
        fseek(fp, (first_data_sector + ((first_cluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec, SEEK_SET);
        /*for (i = 0; i < (bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / 32); i++)
        {
            fread(&currentEntry, sizeof(DirEntry), 1, fp);
            if (currentEntry.DIR_Name[0] == 0x00)
            {
                fseek(fp, -sizeof(DirEntry), SEEK_CUR);
                fwrite(&newEntry, sizeof(DirEntry), 1, fp);
                seek(fp, originalPosition, SEEK_SET);
                return;
            }
        }*/
        printf("\n");
        // move to fat entry offset
        fseek(fp, ((bpb.BPB_RsvdSecCnt * bpb.BPB_BytesPerSec) + (first_cluster * 4)), SEEK_SET);
        fread(&first_cluster, sizeof(int), 1, fp);
    }
    return cluster;
}

unsigned int get_next_cluster(unsigned int current_cluster)
{
    unsigned int fat_offset = current_cluster * 4;
    unsigned int fat_sector = fat_begin_lba + (fat_offset / bpb.BPB_BytesPerSec);
    unsigned int fat_entry_offset = fat_offset % bpb.BPB_BytesPerSec;
    unsigned int fat_entry_value;

    fseek(fp, fat_sector * bpb.BPB_BytesPerSec + fat_entry_offset, SEEK_SET);
    fread(&fat_entry_value, sizeof(fat_entry_value), 1, fp);

    return fat_entry_value & 0x0FFFFFFF;
}

unsigned int get_current_cluster()
{
    unsigned int cluster = (currentEntry.DIR_FstClusLo) | ((currentEntry.DIR_FstClusHi) << 16);
    if (cluster == 0)
    {
        return root_dir_first_cluster;
    }
    return cluster;
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
    int exists = locateDirectory(DIRNAME);
    if (exists == 0)
    {
        unsigned int cluster = get_current_cluster();
        if (cluster == 0)
        {
            cwd.byteOffset = first_data_sector_offset;
            cwd.cluster = bpb.BPB_RootClus;
        }
        else
        {
            cwd.byteOffset = (first_data_sector + ((cluster - 2) * bpb.BPB_SecsPerClus)) * bpb.BPB_BytesPerSec;
            cwd.cluster = cluster;
        }

        if (strcmp(DIRNAME, ".") != 0)
        {
            strcat(cwd.path, "/");
            strcat(cwd.path, DIRNAME);
        }
        else if (strcmp(DIRNAME, "..") == 0)
        {
            int i = strlen(cwd.path);
            unsigned char *current = " ";
            while (strcmp(current, "/") != 0)
            {
                cwd.path[i--] = '\0';
                current = &cwd.path[i];
            }
            cwd.path[i] = '\0';
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
    int i, j;
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
        for (i = 0; i < 16; i++)
        {
            // retrieve information about current entry
            fread(&currentEntry, sizeof(DirEntry), 1, fp);
            // deleted/does not exist/long file name
            if (currentEntry.DIR_Attr == 0x0F || currentEntry.DIR_Name[0] == 0x00)
                continue;
            for (j = 0; j < 11; j++)
            {
                if (currentEntry.DIR_Name[j] == 0x20)
                    currentEntry.DIR_Name[j] = 0x00;
            }
            printf("%s ", currentEntry.DIR_Name);
        }
        printf("\n");
        // move to fat entry offset
        fseek(fp, ((bpb.BPB_RsvdSecCnt * bpb.BPB_BytesPerSec) + (currentCluster * 4)), SEEK_SET);
        fread(&currentCluster, sizeof(int), 1, fp);
    }
    fseek(fp, originalPosition, SEEK_SET);
}

// Create
void mkdir(char *DIRNAME)
{
}

/* this uses the same search method as locateDirectory() and ls()
but pushes a new directory entry instead of showing what is already there*/
void creat(char *FILENAME)
{ // Check if a file with the same name already exists
    if (locateDirectory(FILENAME) != 1)
    {
        printf("File already exists\n");
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
        memcpy(new_entry.DIR_Name, FILENAME, strlen(FILENAME));
        new_entry.DIR_Attr = 0x20;
        // Set the first cluster to the first free cluster
        new_entry.DIR_FstClusLo = 0x0;
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
                fread(&currentEntry, sizeof(DirEntry), 1, fp);
                if (currentEntry.DIR_Name[0] == 0x00)
                {
                    fseek(fp, -sizeof(DirEntry), SEEK_CUR);
                    fwrite(&new_entry, sizeof(DirEntry), 1, fp);
                    fseek(fp, originalPosition, SEEK_SET);
                }
            }
            printf("File created successfully\n");
            return;
        }
    }
}
void cp(char *FILENAME, unsigned int TO) {}

// Read
void open(char *FILENAME, int FLAGS) {
    fseek(fp, (first_data_sector + ((currentCluster - 2) * bpb.BPB_SecsPerClus)), SEEK_SET);
    struct DirEntry entry;
    bool found_file = false;
    while (fread(&entry, BYTES_PER_DIR_ENTRY, 1, fp) == 1) {
        if (entry.DIR_Name[0] == 0x00) {
            // This entry and all subsequent entries are unused
            break;
        } else if (entry.DIR_Name[0] == 0xE5) {
            // This entry is unused
            continue;
        } else if (entry.DIR_Attr == 0x0F) {
            // This is a long filename entry
            continue;
        } else {
            // This is a regular file or directory entry
            char filename[11];
            memset(filename, 0, sizeof(filename));
            strncpy(filename, entry.DIR_Name, 11);
            if (strcmp(filename, argv[2]) == 0) {
                found_file = true;
                break;
            }
        }
    }
	
    if (!found_file) {
        printf("File not found: %s\n", argv[2]);
        fclose(fp);
        return -1;
    }

    // Open the file and seek to the start of its data
    uint32_t cluster = entry.DIR_FstClusLo;
	unsigned int CLUSTER_SIZE = (BPB_BytesPerSec * BPB_SecsPerClus);
    fseek(fp, (cluster - 2) * CLUSTER_SIZE, SEEK_SET);
	
    printf("File contents:\n");

    // Read and print each cluster of the file's data
    while (cluster < 0x0FFFFFF8) {
        uint8_t buffer[CLUSTER_SIZE];
        fread(buffer, CLUSTER_SIZE, 1, fp);
        printf("%.*s", CLUSTER_SIZE, buffer);
        cluster = *((uint32_t *) buffer + (CLUSTER_SIZE / 4) - 1);
        fseek(fp, (cluster - 2) * CLUSTER_SIZE, SEEK_SET);
    }

    fclose(fp);

    return 0;
}
}
void close(char *FILENAME) {}
void lsof(void) {}

/* Just prints out the current entry's file size
after checking if the file is a directory or a
file and if it exists or not using locateDirectory() */
void size(char *FILENAME)
{
    int i = locateDirectory(FILENAME);
    if (i == 1 | i == 0)
    {
        printf("%s\n", "Error, is directory or does not exist");
    }
    else
    {
        printf("%s %d %s\n", "File is", currentEntry.DIR_FileSize, "bytes");
    }
}
void lseek(char *FILENAME, unsigned int OFFSET) {}
void read(char *FILENAME, unsigned int size) {}

// Update
void write(char *FILENAME, char *STRING) {}
// void rename(char *FILENAME, char *NEW_FILENAME) {}

// Delete
void rm(char *FILENAME) {}
void rmdir(char *DIRNAME) {}

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