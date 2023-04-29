# Project 3 Group 29 COP4610

## Group Members

-   Daniel Pijeira
-   Ethan Burke
-   Abraham Beltran

## Division of Labor

-   Part 1: Mount image file. By April 1
    -   Contributors: Daniel Pijeira & Ethan Burke
    -   Components:
        -   info Ethan Burke
        -   exit Daniel Pijeira
-   Part 2: Navigation By April 5
    -   Contributors: Abraham Beltran & Ethan Burke
    -   Components:
        -   cd [DIRNAME] Abraham Beltran
        -   ls Ethan Burke
-   Part 3: Create By April 12
    -   Contributors: Daniel Pijeira & Abraham Beltran
    -   Components:
        -   mkdir [DIRNAME] Daniel Pijeira
        -   creat [FILENAME] Abraham Beltran
        -   cp [FILENAME] [TO] Daniel Pijeira & Abraham Beltran
-   Part 4: Read By April 19
    -   Contributors: Abraham Beltran & Ethan Burke
    -   Components:
        -   open [FILENAME] [FLAGS] Abraham Beltran
        -   close [FILENAME] Ethan Burke
        -   lsof Abraham Beltran
        -   size [FILENAME] Ethan Burke
        -   lseek [FILENAME] [OFFSET] Abraham Beltran
        -   read [FILENAME] [SIZE] Ethan Burke
-   Part 5: Update By April 26

    -   Contributors: Daniel Pijeira & Abraham Beltran
    -   Components:
        -   write [FILENAME] [STRING] Daniel Pijeira
        -   rename[FILENAME] [NEW_FILENAME] Abraham Beltran

-   Part 6: Delete By April 28 (due date of project)
    -   Contributors: Ethan Burke & Daniel Pijeira
    -   Components:
        -   rm [FILENAME] Ethan Burke
        -   rmdir [DIRNAME] Daniel Pijeira

## Division of Labor (after done)

-   Part 1: Mount image file. By April 1
    -   Contributors: Daniel Pijeira & Abraham Beltran
    -   Components:
        -   info Abraham Beltran
        -   exit Daniel Pijeira
-   Part 2: Navigation By April 5
    -   Contributors: Abraham Beltran & Ethan Burke
    -   Components:
        -   cd [DIRNAME] Abraham Beltran
        -   ls Ethan Burke
-   Part 3: Create By April 12
    -   Contributors: Daniel Pijeira & Abraham Beltran
    -   Components:
        -   mkdir [DIRNAME] Daniel Pijeira
        -   creat [FILENAME] Abraham Beltran
        -   cp [FILENAME] [TO] Daniel Pijeira & Abraham Beltran
-   Part 4: Read By April 19
    -   Contributors: Abraham Beltran & Ethan Burke
    -   Components:
        -   open [FILENAME] [FLAGS] Abraham Beltran
        -   close [FILENAME] Ethan Burke
        -   lsof Abraham Beltran
        -   size [FILENAME] Abraham Beltran
        -   lseek [FILENAME] [OFFSET] Abraham Beltran
        -   read [FILENAME] [SIZE] Ethan Burke
-   Part 5: Update By April 26

    -   Contributors: Daniel Pijeira & Abraham Beltran
    -   Components:
        -   write [FILENAME] [STRING] Daniel Pijeira
        -   rename[FILENAME] [NEW_FILENAME] Abraham Beltran

-   Part 6: Delete By April 28 (due date of project)
    -   Contributors: Ethan Burke & Daniel Pijeira
    -   Components:
        -   rm [FILENAME] Ethan Burke
        -   rmdir [DIRNAME] Daniel Pijeira

## Files

-   .gitignore - Ignores the binary files we have on our systems from compiling
-   makefile - Used to compile and generate the executable
-   README.md - This readme file
-   filesys.c - This is the main file containing all of the functions for the project

## Compiling using the makefile

Run the command `make filesys` from your terminal in the directory where you keep the files. To run the project run `./filesys fat32.img` where you keep the fat32.img and filesys.c

## Bugs & Incomplete Section

Could not finish implementations of cp, read, write, rm, rmdir

Implementation of lseek does not check for file size being bigger than the offset size specified.
Current implementation of cp does not recognize files and directories properly and will occasionaly copy or crash.
Current version of read will throw an infinite loop, print out everything available on the image, or do literally nothing.
Current version of rm will not remove files but shows the possible structure the function could've taken given more time.
lseek will throw a segmentation fault if you do not specify offset size.
