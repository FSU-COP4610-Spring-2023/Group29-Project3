// starter file provided by operating systems ta's

// include libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 40

#define PATH_SIZE 4096          // max possible size for path string.
#define OPEN_FILE_TABLE_SIZE 10 // max files for files.
#define MAX_NAME_LENGTH 11      // 11 in total but 3 for extensions, we only use 8.

FILE *fp;
FILE *fp1;
FILE *fp2;

// data structures for FAT32
// Hint: BPB, DIR Entry, Open File Table -- how will you structure it?
/*typedef struct __attribute__((packed))
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
} DirEntry;*/

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

typedef struct __attribute__((__packed__))
{
    unsigned char DIR_Name[11]; // directory name
    unsigned char DIR_Attr;     // directory attribute count
    unsigned short DIR_FirstClusterHigh;
    unsigned short DIR_FirstClusterLow;
    unsigned int DIR_FileSize; // directory size
} DirEntry;
// stack implementaiton -- you will have to implement a dynamic stack
// Hint: For removal of files/directories

typedef struct
{
    char path[PATH_SIZE]; // path string
    // add a variable to help keep track of current working directory in file.
    int currentCluster;
    long byteOffset;
    long rootOffset;
    // Hint: In the image, where does the first directory entry start?

} CWD;

typedef struct{
    char* path;
    DirEntry dirEntry;
    unsigned int offset;
    unsigned int firstCluster; // First cluster location
    unsigned int firstClusterOffset; // Offset of first cluster in bytes
    int mode; // 2=rw, 1=w, 0=r, -1=not open (i.e, -1 is a "free" spot)
} File;

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
// void rename(char *FILENAME, char *NEW_FILENAME);
void rm(char *FILENAME);
void rmdir(char *DIRNAME);

// global variables
CWD cwd;
FILE *fp; // file pointers
BPB bpb;  // boot sector information
DirEntry currentEntry;
//Global variables from supplementary slides
int rootDirSectors;
int firstDataSector;
long firstDataSectorOffset;
File openFiles[10];
int numFilesOpen = 0;

// These variables should be all we need for accessing, modifying, and working with the FAT32 files
// Partition_LBA_Begin can be found using the Microsoft docs and is essential to finding proper locations
/*unsigned long fat_begin_lba = bpb.Partition_LBA_Begin + bpb.BPB_RsvdSecCnt;
unsigned long cluster_begin_lba = bpb.Partition_LBA_Begin + bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);
unsigned char sectors_per_cluster = bpb.BPB_SecsPerClus;
unsigned long root_dir_first_cluster = bpb.BPB_RootClus;*/

int main(int argc, char *argv[])
{
    // error checking for number of arguments.
    if (argc != 2)
    {
        printf("invalid number of arguments.\n");
        return -1; // return error indicator if number of args is not 2.
    }
    // read and open argv[1] in file pointer.
    if ((fp = fopen(argv[1], "r+")) == NULL)
    { // r+ stands for read,write,append.
        printf("%s does not exist\n.", argv[1]);
        return -1;
    }

    // obtain important information from bpb as well as initialize any important global variables
    memset(cwd.path, 0, PATH_SIZE);
    fread(&bpb, sizeof(BPB), 1, fp);
    rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytesPerSec - 1)) / bpb.BPB_BytesPerSec;
    firstDataSector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + rootDirSectors;
    firstDataSectorOffset = firstDataSector * bpb.BPB_BytesPerSec;
    cwd.rootOffset = firstDataSectorOffset;
    cwd.byteOffset = cwd.rootOffset;
    cwd.currentCluster = bpb.BPB_RootClus;
    memset(cwd.path, 0, PATH_SIZE);
    //strcat(cwd.path, argv[1]);
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
            if(tokens->items[1] != NULL)
            {
                cd(tokens->items[1]);
            }
            else
            {
                printf("Error: Must enter directory to switch to.\n");
            }
        }
        else if (strcmp(tokens->items[0], "ls") == 0)
        {
            ls();
        }
        else if (strcmp(tokens->items[0], "mkdir") == 0)
        {
            if(tokens->items[1] != NULL)
            {
                mkdir(tokens->items[1]);
            }
            else
            {
                printf("Error: Must enter name of directory.");
            }
            
        }
        else if (strcmp(tokens->items[0], "creat") == 0)
        {
            if(tokens->items[1] != NULL)
            {
                creat(tokens->items[1]);
            }
            else
            {
                printf("Error: Must enter name of directory.");
            }
            
        }
        // add_to_path(tokens->items[0]); // move this out to its correct place;
        free(input);
        free_tokens(tokens);
    }

    return 0;
}

// helper functions -- to navigate file image

// converts the LBA address we get from fread() and turns it into an int for offset
int LBAToOffset(unsigned int sector)
{
    if (sector == 0)
        sector = 2;
    return ((sector - 2) * bpb.BPB_BytesPerSec) + (bpb.BPB_BytesPerSec * bpb.BPB_RsvdSecCnt) + (bpb.BPB_NumFATs * bpb.BPB_FATSz32 * bpb.BPB_BytesPerSec);
}
// commands -- all commands mentioned in part 2-6 (17 cmds)

// Mount
/* This function basically just takes in the file struct
bpb as initialized earlier and then prints out all of the info
based off of the mounted image. The image is opened and read
by using fread() and fopen() in main.*/
void info()
{
    int totalDataSectors = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + rootDirSectors);
    int totalClusters = totalDataSectors / bpb.BPB_SecsPerClus;
    printf("Position of root: %d\n", bpb.BPB_RootClus);
    printf("Bytes per sector: %d\n", bpb.BPB_BytesPerSec);
    printf("Sectors per cluster: %d\n", bpb.BPB_SecsPerClus);
    printf("# of clusters in data region: %d\n", totalClusters);
    printf("# of entries in one fat: %d\n", ((bpb.BPB_FATSz32 * bpb.BPB_BytesPerSec) / 4));
    printf("Size of image (in bytes): %d\n", bpb.BPB_TotSec32 * bpb.BPB_BytesPerSec);
}

// Navigation
void cd(char *DIRNAME)
{
    int i;
    if (strncmp(currentEntry.DIR_Name, "..", 2) == 0)
    { // finds if it matches then uses the offset to find the directory
        int offset = LBAToOffset(currentEntry.DIR_FirstClusterLow);
        cwd.currentCluster = currentEntry.DIR_FirstClusterLow;
        fseek(fp, offset, SEEK_SET);
        fread(&currentEntry, 32, 16, fp);
        return;
    }
    int offset = LBAToOffset(DIRNAME);
    cwd.currentCluster = DIRNAME;
    // change current cluster and reread information
    fseek(fp, offset, SEEK_SET);
    fread(&currentEntry, 32, 16, fp);
}

void ls(void)
{
    int offset = LBAToOffset(cwd.currentCluster);
    // get offset and then get data
    fseek(fp, offset, SEEK_SET);
    int i, j;
    for (i = 0; i < 16; i++)
    {
        fread(&currentEntry, sizeof(DirEntry), 1, fp);
        // iterate through data structure
        if (currentEntry.DIR_Attr == 0x0F || currentEntry.DIR_Name == 0x00)
        {
            for (j = 0; j < 11; j++)
            {
                if (currentEntry.DIR_Name[j] == 0x20)
                    currentEntry.DIR_Name[j] = 0x00;
                // iterate through entries in the cluster and print
            }
            printf("%s \n", currentEntry.DIR_Name);
        }
    }
}

// Create
void mkdir(char *DIRNAME) 
{
    
}
void creat(char *FILENAME) 
{

}
void cp(char *FILENAME, unsigned int TO) 
{

}

// Read
void open(char *FILENAME, int FLAGS) {}
void close(char *FILENAME) {}
void lsof(void) {}
void size(char *FILENAME) {}
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