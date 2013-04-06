///////////////////////////////////////////////////////////////////////////////
// PROGRAM 3 - RAID
// Title:
// Files:
// Semester: CS537 Spring 2013
//
// PAIR PROGRAMMERS
//
// Pair Partner: (Ted) Tianchu Huang thuang33@wisc.edu
// CS Login: Tianchu
// Lecturer's Name: Michael Swift
//
// Pair Partner: Tyson Williams tjwilliams4@wisc.edu
// CS Login: twilliam
// Lecturer's Name: Michael Swift
//
// Pair Partner: Adam Thorson adthorson@wisc.edu
// CS Login: thorson
// Lecturer's Name: Michael Swift
//
//////////////////////////// 80 columns wide //////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "disk.h"
#include "disk-array.h"

#define TRUE 1
#define FALSE 0

void toParity(int parityDisk, int blockNumber, int dataDisk, char * dataBlock);
char fromParity(int blockNumber, int diskNumber, disk_array_t my_disk_array);

int value = 0;
int lba = 0;
int commandSize = 0;
int commandDisk = 0;

char* error_msg = "Error\n";
int counter     = 0;
int level       = -1;
int strip       = -1;
int disks       = -1;
int size        = -1;
char* trace     = NULL;
int verbose     = 0;
int flag       = 0;
int exit_flag   = 0;
char str[100];
//disk_array_t my_disk_array = NULL;
FILE * trace_file = NULL;
int * working_disks = NULL;

// used to load respective values in blocks of stripe
char result; //the bitwise result of XOR's


/*
 * do RAID 0
 *
 *@param
 *
 */
void doRaid0() {
    disk_array_t my_disk_array = disk_array_create("MyVirtualDiskArray", disks, size);
	if (!my_disk_array) {
		//debugging
		printf("%s\n","Unable to Create Virtual Disks");
        
		write(STDERR_FILENO, error_msg, strlen(error_msg));
		exit(-1);
	}
    
    char *data;
    data = malloc(1024);
    
    while (fgets(str, 100, trace_file) != NULL) {
        printf("%s", str);
        
        //for each line parse and detect what command we have
        //for this purpose, "line" is the string on the line from the trace file
        char *command = NULL;
		command = (char*) malloc(512);
		if(str[strlen(str) - 1] == '\n')
			str[strlen(str) - 1] = '\0'; //remove newline char
        
        char * commandLine[5];
        
        command = strtok(str, " "); //split string on space delimiter into tokens
        
		int i = 0;
		while( command != NULL ) {
            commandLine[i] = command;
			i++;
            command = strtok( NULL, " " );
        }
        
        if (strcmp("READ", commandLine[0]) == 0) { //READ LBA SIZE
			int numberOfReads = atoi(commandLine[2]);
			int currentLBA = atoi(commandLine[1]);
			int j;
			int blockNumber; //starting LBA
			int diskNumber;
			int stripLayer;
			int blockOfStrip;
			int temp;
            int printedData;
			
			for (j = currentLBA; j < (currentLBA + numberOfReads) /*size*/; j++) { // number of blocks we have to write to
				temp = j / strip;
				stripLayer = temp / disks;
				blockOfStrip = j % strip;
				blockNumber = stripLayer * strip + blockOfStrip;
				diskNumber = temp % disks; //algorithm to calculate the disk we read from
                
                int readCheck = disk_array_read( my_disk_array, diskNumber, blockNumber, data );
				if (readCheck == -1) {
                    printf("ERROR ");
                }
                else {
                    printedData = atoi(data);
                    printf("%d ", printedData);
                }
			}
		}
		
		else if (strcmp("WRITE", commandLine[0]) == 0) { //WRITE LBA SIZE VALUE
			char *data = commandLine[3];
			int numberOfWrites = atoi(commandLine[2]);
			int currentLBA = atoi(commandLine[1]);
			int j;
			int blockNumber; //starting LBA
			int diskNumber;
			int stripLayer;
			int blockOfStrip;
			int temp;
            int writeCheck;
			
			for (j = currentLBA; j < (currentLBA + numberOfWrites) /*size*/; j++) { // number of blocks we have to write to
				temp = j / strip;
				stripLayer = temp/disks;
				blockOfStrip = j % strip;
				blockNumber = stripLayer * strip + blockOfStrip;
				diskNumber = temp % disks; //algorithm to calculate the disk we write to
                
				writeCheck = disk_array_write( my_disk_array, diskNumber, blockNumber, data );
			}
            if (writeCheck == -1) {
                printf("ERROR ");
            }
		}
		
		else if (strcmp("FAIL", commandLine[0]) == 0) { //FAIL DISK
            disk_array_fail_disk( my_disk_array, atoi(commandLine[1]));
            working_disks[atoi(commandLine[1])] = FALSE;
		}
		
		else if (strcmp("RECOVER", commandLine[0]) == 0) { //RECOVER DISK
            disk_array_recover_disk( my_disk_array, atoi(commandLine[1]));
		}
		
        else if (strcmp("END", commandLine[0]) == 0){ // END
            disk_array_print_stats(my_disk_array);
            break;
        }
		
		else {
			//debugging
			printf("%s\n","Trace File Error");
            
			write(STDERR_FILENO, error_msg, strlen(error_msg));
			exit(-1);
		}
        free(command);
    }
    
    
}


/*
 * do RAID 5
 *
 *@param
 *
 */
void doRaid5() {
    
	disk_array_t my_disk_array = disk_array_create("MyVirtualDiskArray", disks, size);
	if (!my_disk_array) {
		//debugging
		printf("%s\n","Unable to Create Virtual Disks");
        
		write(STDERR_FILENO, error_msg, strlen(error_msg));
		exit(-1);
	}
	
    char *data;
	int blockNumber; //starting LBA
	int diskNumber;
	int stripLayer;
	int blockOfStrip;
	int temp;
    data = malloc(1024);
	int writeCheck;
    char blocks[disks][1024];
    int doubleFail = -1;
	int writeFailFlag=0;
	
    
    while (fgets(str, 100, trace_file) != NULL) {//for each line
		printf("%s", str);
		
        //parse and detect what command we have
        //for this purpose, "line" is the string on the line from the trace file
        char *command = NULL;
		command = (char*) malloc(512);
		if(str[strlen(str) - 1] == '\n')
			str[strlen(str) - 1] = '\0'; //remove newline char
        
        char * commandLine[5];
        
        command = strtok(str, " "); //split string on space delimiter into tokens
        
		int i = 0;
		while( command != NULL ) {
            commandLine[i] = command;
			i++;
            command = strtok( NULL, " " );
        }		
		
        if (strcmp("READ", commandLine[0]) == 0) { //READ LBA SIZE
			/*
             in: currentLBA
             
             //the following equation is used to adjust for scale for RAID5
             int increment = (currentLBA/disks + 1) + (currentLBA/(disks*disks) + 1);
             currentLBA += increment;
             out: currentLBA
             */
            int currentLBA = atoi(commandLine[1]);
            int q1 = currentLBA / (strip * disks);
            int q2 = currentLBA / (strip * disks * (disks - 1));
            currentLBA += (q1 + q2 + 1) * strip;
            
            
			int numberOfReads = atoi(commandLine[2]);
            
			int j;
            int threshold = currentLBA + numberOfReads;
            if (threshold > size * (disks -1))
                //we need this so we do not write more than the total amount of blocks in all disks
                threshold = size * (disks -1); //ex if there are only 10 blocks, we can't write >10 times
            

            
			for (j = currentLBA; j < threshold; j++) { // number of blocks we have to write to
                     
                
				temp = j / strip;
				stripLayer = temp / disks; //make it seem like we do not have the parity disk
				blockOfStrip = j % strip;
				blockNumber = stripLayer * strip + blockOfStrip;
				diskNumber = temp % disks; //algorithm to calculate the disk we read from
				int numberOfStripes = size/strip;
				int doNotRead = 0;
                
				int p = 0;
				for(i = 0; i < (numberOfStripes);i++){
					if(p == disks){
						p = 0;
					}
					if((stripLayer == i) && (diskNumber == p)){
						printf("reading from parity: block skipped\n");
						if(threshold != size * (disks - 1))
							threshold++; //we adjust for not writing to last block
						doNotRead = 1;
						break;
					}
					p++;
				}
				
				if (doNotRead != 1) {
					int readCheck = disk_array_read( my_disk_array, diskNumber, blockNumber, data );
					
					//if disk_array_read returns -1, we tried to read froma failed disk
					//if so, we need to try and recover the old info
                    if (readCheck == -1) {
                        
                        int i, k;
                        for (i = 0; i < disks; i++) {
                            if (i != diskNumber) { // we don't want to read old datadisk data
                                disk_array_read(my_disk_array, i, blockNumber, blocks[i]); //if this doesnt work change "block[i]" into "&block[i][0]"
                                printf("Reading in for parity: %s\n", blocks[i]);
                            }
                        }
                        for (i = 0; i < 1024; i++) {
                            char parity = 0;
                            for (k = 0; k < disks; k++) {
                                if (k != diskNumber)
                                    parity = parity ^ blocks[k][i];
                            }
                            blocks[diskNumber][i] = parity;
                        }
                        
                        printf("Recovered data: %s\n", blocks[diskNumber]);
                    }
                    
                    else {
                        printf("%s ", data);
                    }
				}
				doNotRead = 0;
			}
		}
		
		else if ( strcmp("WRITE", commandLine[0]) == 0 ) { //WRITE LBA SIZE VALUE
			/*
             in: currentLBA
             
             //the following equation is used to adjust for scale for RAID5
             int increment = (currentLBA/disks + 1) + (currentLBA/(disks*disks) + 1);
             currentLBA += increment;
             out: currentLBA
             */
            int currentLBA = atoi(commandLine[1]);
            int q1 = currentLBA / (strip * disks);
            int q2 = currentLBA / (strip * disks * (disks - 1));
            currentLBA += (q1 + q2 + 1) * strip;
            
            
            
			char *data = commandLine[3];
			int numberOfWrites = atoi(commandLine[2]);
            
            int i, j, z, parityDisk;
            int threshold = currentLBA + numberOfWrites;
            if (threshold > size * (disks-1)) { //we need this so we do not write more than the total amount of blocks in all disks
                threshold = size * (disks-1); //ex if there are only 10 blocks, we can't write >10 times
            }
		
			for (j = currentLBA; j < threshold; j++) { // number of blocks we have to write to
				
				temp = j / strip;
				stripLayer = temp/disks;
				blockOfStrip = j % strip;
				blockNumber = stripLayer * strip + blockOfStrip;
				diskNumber = temp % (disks); //algorithm to calculate the disk we write to
				int doNotWrite = 0;
				
				int numberOfStripes = size/strip;
				int p = 0;
				for(i = 0; i < (numberOfStripes);i++){
					if (p == disks){
						p = 0;
					}
					if((stripLayer == i) && (diskNumber == p)){
						printf("writing to parity: block skipped\n");
						if(threshold != size * (disks - 1)){
							threshold++; //we adjust for not writing to last block
                            doNotWrite = 1;
                            break;
						}
                    }
                    p++;
                }
                
                if(doNotWrite != 1){
                    // TO PARITY: computing parity to store back in parity disk
                    
                    char blocks[disks][1024];    
                    
                    parityDisk = stripLayer % disks;

                    for (i = 0; i < disks; i++) {
                        if ((i != parityDisk) && (i != diskNumber)) // we don't want to read either parityDisk or old datadisk
                            disk_array_read(my_disk_array, i, blockNumber, &blocks[i][0]); //if this doesnt work change "block[i]" into "&block[i][0]"
                    }
                    
                    memcpy(&blocks[diskNumber][0], data, 1024);
                    
                    for (i = 0; i < 1024; i++) {
                        int parity = 0;
                        for (z = 0; z < disks; z++) {
                            if (z == parityDisk)
                                continue; //do nothing
                            parity = parity ^ blocks[z][i];
                        }
                        blocks[parityDisk][i] = parity;
                    }
                    
                    disk_array_write(my_disk_array, parityDisk, blockNumber, &blocks[parityDisk][0]);
                    
                    
                    //write to updating disk
                    writeCheck = disk_array_write( my_disk_array, diskNumber, blockNumber, data);
                    
                    //if we detect that we are writing to a failed disk, rewrite 0 and print ERROR
                    if (writeCheck == -1) {
                        disk_array_write( my_disk_array, diskNumber, blockNumber, 0);
                        writeFailFlag = 1;
                        //printf("ERROR: writing into failed disk\n");
                        //return;
                    }
                }
                doNotWrite = 0;
                
            }
            if (writeFailFlag)
                printf("ERROR: writing into failed disk\n");
        }
        
        else if(strcmp("FAIL", commandLine[0]) == 0) { //FAIL DISK
            disk_array_fail_disk( my_disk_array, atoi(commandLine[1]));
            working_disks[atoi(commandLine[1])] = FALSE;
            doubleFail++;
            
        }
        
        else if(strcmp("RECOVER", commandLine[0]) == 0) { //RECOVER DISK
            
            int recoveredDisk = atoi(commandLine[1]);
            disk_array_recover_disk( my_disk_array, recoveredDisk); //we clear the disk
            working_disks[recoveredDisk] = TRUE;
            
            
            int j;
            for (j = 0; j < size; j++) { //for every block within a disk
                fromParity(j, atoi(commandLine[1]), my_disk_array);
            }
        }
        
        else if(strcmp("END", commandLine[0]) == 0){ // END
            break;
        }
        
        else{
            //debugging
            printf("%s\n","Trace File Error");
            
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(-1);
        }
        free(command);
        
        doubleFail--;
    }
}





/*
 * do RAID 4
 *
 *@param
 *
 */
void doRaid4() {
    
    disk_array_t my_disk_array = disk_array_create("MyVirtualDiskArray", disks, size);
    if (!my_disk_array) {
        //debugging
        printf("%s\n","Unable to Create Virtual Disks");
        
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    char *data;
    int blockNumber; //starting LBA
    int diskNumber;
    int stripLayer;
    int blockOfStrip;
    int temp;
    int numberOfReads;
    data = malloc(1024);
    int writeCheck;
    int doubleFail = -1;
    char blocks[disks][1024];
    int writeFailFlag=0;
    
    while (fgets(str, 100, trace_file) != NULL) {
        printf("%s", str);
        
        //for each line parse and detect what command we have
        //for this purpose, "line" is the string on the line from the trace file
        char *command = NULL;
        command = (char*) malloc(512);
        if(str[strlen(str) - 1] == '\n')
            str[strlen(str) - 1] = '\0'; //remove newline char
        
        char * commandLine[5];
        
        command = strtok(str, " "); //split string on space delimiter into tokens
        
        int i = 0;
        while ( command != NULL ) {
            //commandLine[i] = malloc(8 * sizeof(char));
            //printf("%s\n",command);
            commandLine[i] = command;
            i++;
            command = strtok( NULL, " " );
        }
        
        if (strcmp("READ", commandLine[0]) == 0) { //READ LBA SIZE
            numberOfReads = atoi(commandLine[2]);
            int currentLBA = atoi(commandLine[1]);
            int j;
            int threshold = currentLBA + numberOfReads;
            if (threshold > size * (disks-1 /*excluding parityDisk*/ ))
                //we need this so we do not write more than the total amount of blocks in all disks
                threshold = size * (disks-1); //ex if there are only 10 blocks, we can't write >10 times
            
            
            for (j = currentLBA; j < threshold; j++) { // number of blocks we have to write to
                temp = j / strip;
                stripLayer = temp / (disks-1); //make it seem like we do not have the last disk
                blockOfStrip = j % strip;
                blockNumber = stripLayer * strip + blockOfStrip;
                diskNumber = temp % (disks-1); //algorithm to calculate the disk we read from
                
                int readCheck = disk_array_read( my_disk_array, diskNumber, blockNumber, data );
                
                
                //if disk_array_read returns -1, we tried to read froma failed disk
                //if so, we need to try and recover the old info
                if (readCheck == -1) {
                    
                    //fromParity(j (blockNumber), currentLBA (dataDisk) );
                    
                    
                    int i, k;
                    for (i = 0; i < disks; i++) {
                        if (i != diskNumber) { // we don't want to read old datadisk data
                            disk_array_read(my_disk_array, i, blockNumber, blocks[i]); //if this doesnt work change "block[i]" into "&block[i][0]"
                            printf("Reading in for parity: %s\n", blocks[i]);
                        }
                    }
                    for (i = 0; i < 1024; i++) {
                        char parity = 0;
                        for (k = 0; k < disks; k++) {
                            if (k != diskNumber)
                                parity = parity ^ blocks[k][i];
                        }
                        blocks[diskNumber][i] = parity;
                    }
                    //disk_array_write(my_disk_array, currentLBA, j, blocks[currentLBA]);
                    //if this doesnt work change "block[parityDisk]" into "&block[parityDisk][0]"
                    
                    printf("Recovered data: %s\n", blocks[diskNumber]);
                    
                }
                
                else {
                    //printedData = atoi(data);
                    printf("%s ", data);
                }
            }
        }
        
        
        else if ( strcmp("WRITE", commandLine[0]) == 0 ) { //WRITE LBA SIZE VALUE
            char *data = commandLine[3];
            int numberOfWrites = atoi(commandLine[2]);
            int currentLBA = atoi(commandLine[1]);
            int i, j, z, parityDisk;
            int threshold = currentLBA + numberOfWrites;
            if (threshold > size * (disks-1)) { //we need this so we do not write more than the total amount of blocks in all disks
                threshold = size * (disks-1); //ex if there are only 10 blocks, we can't write >10 times
            }
            
            for (j = currentLBA; j < threshold; j++) { // number of blocks we have to write to
                temp = j / strip;
                stripLayer = temp / (disks-1);
                blockOfStrip = j % strip;
                blockNumber = stripLayer * strip + blockOfStrip;
                diskNumber = temp % (disks-1); //algorithm to calculate the disk we write to
                
                parityDisk = disks - 1;
                
                // TO PARITY: computing parity to store back in parity disk
                
                char blocks[disks][1024];
                
                for (i = 0; i < disks; i++) {
                    if ((i != parityDisk) && (i != diskNumber)) { // we don't want to read either parityDisk or old datadisk
                        disk_array_read(my_disk_array, i, blockNumber, &blocks[i][0]); //if this doesnt work change "block[i]" into "&block[i][0]"
                    }
                }
                memcpy(&blocks[diskNumber][0], data, 1024);
                
                for (i = 0; i < 1024; i++) {
                    char parity = 0;
                    for (z = 0; z < disks; z++) {
                        if (z == parityDisk)
                            continue; //do nothing
                        parity = parity ^ blocks[z][i];
                    }
                    blocks[parityDisk][i] = parity;
                }
                
                disk_array_write(my_disk_array, parityDisk, blockNumber, &blocks[parityDisk][0]);
                
                printf("new parity is: %s\n", blocks[parityDisk]);
                //if this doesnt work change "block[parityDisk]" into "&block[parityDisk][0]"
                
                //write to updating disk
                writeCheck = disk_array_write( my_disk_array, diskNumber, blockNumber, data);
                
                //if we detect that we are writing to a failed disk, rewrite 0 and print ERROR
                if (writeCheck == -1) {
                    disk_array_write( my_disk_array, diskNumber, blockNumber, 0);
                    writeFailFlag = 1;
                }
                
            }
            if (writeFailFlag)
                printf("ERROR: writing into failed disk\n");
        }
        
        else if (strcmp("FAIL", commandLine[0]) == 0) { //FAIL DISK
            disk_array_fail_disk( my_disk_array, atoi(commandLine[1]));
            working_disks[atoi(commandLine[1])] = FALSE;
            doubleFail++;
            
        }
        
        else if (strcmp("RECOVER", commandLine[0]) == 0) { //RECOVER DISK
            
            int recoveredDisk = atoi(commandLine[1]);
            disk_array_recover_disk( my_disk_array, recoveredDisk); //we clear the disk
            working_disks[recoveredDisk] = TRUE;
            
            
            int j;
            for (j = 0; j < size; j++) { //for every block within a disk
                fromParity(j, atoi(commandLine[1]), my_disk_array);
            }
        }
        
        else if (strcmp("END", commandLine[0]) == 0) { // END
            disk_array_print_stats(my_disk_array);
            break;
        }
        
        else {
            //debugging
            printf("%s\n","Trace File Error");
            
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(-1);
        }
        free(command);
        
        doubleFail--;
    }
}

/*
 * do RAID 10
 *
 *@param
 *
 */
void doRaid10() {
    disk_array_t my_disk_array = disk_array_create("MyVirtualDiskArray", disks, size);
    if (!my_disk_array) {
        //debugging
        printf("%s\n","Unable to Create Virtual Disks");
        
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    char *data;
    data = malloc(1024);
    
    if ((disks % 2) == 1) {
        printf("Odd number of disks");
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    while (fgets(str, 100, trace_file) != NULL) {//for each line
        //parse and detect what command we have
        //for this purpose, "line" is the string on the line from the trace file
        char *command = NULL;
        command = (char*) malloc(512);
        if(str[strlen(str) - 1] == '\n')
            str[strlen(str) - 1] = '\0'; //remove newline char
        
        char * commandLine[5];
        
        command = strtok(str, " "); //split string on space delimiter into tokens
        
        int i = 0;
        while( command != NULL ) {
            commandLine[i] = command;
            i++;
            command = strtok( NULL, " " );
        }
        int j;
        for (j=0; j<i; j++) {
            printf("%s ",commandLine[j]);
        }
        printf("\n");
        
        
        if (strcmp("READ", commandLine[0]) == 0){ //READ LBA SIZE
            int numberOfReads = atoi(commandLine[2]);
            int currentLBA = atoi(commandLine[1]);
            int j;
            int blockNumber; //starting LBA
            int diskNumber;
            int stripLayer;
            int blockOfStrip;
            int temp;
            
            for (j = currentLBA; j < (currentLBA + numberOfReads) /*size*/; j++) { // number of blocks we have to write to
                int flag1 = 1;
                
                temp = j / strip;
                
                stripLayer = temp / (disks / 2);
                blockOfStrip = j % strip;
                blockNumber = stripLayer * strip + blockOfStrip;
                
                diskNumber = 2 * (temp % (disks / 2)); //algorithm to calculate the disk we write to
                
                disk_array_read( my_disk_array, diskNumber, blockNumber, data );
                
                //has disk failed?
                if (!working_disks[diskNumber]) {
                    ++diskNumber;
                    disk_array_read( my_disk_array, diskNumber, blockNumber, data );
                    if (!working_disks[diskNumber]) {
                        printf("ERROR ");
                        flag1 = 0;
                    }
                }
                if (flag1)
                    printf("%d ", atoi(data));
            }
        }
        
        
        else if (strcmp("WRITE", commandLine[0]) == 0) { //WRITE LBA SIZE VALUE
            char *data = commandLine[3];
            int numberOfWrites = atoi(commandLine[2]);
            int currentLBA = atoi(commandLine[1]);
            int j;
            int blockNumber; //starting LBA
            int diskNumber;
            int stripLayer;
            int blockOfStrip;
            int temp;
            
            for (j = currentLBA; j < (currentLBA + numberOfWrites) /*size*/; j++) { // number of blocks we have to write to
                temp = j / strip;
                
                stripLayer = temp / (disks / 2);
                blockOfStrip = j % strip;
                blockNumber = stripLayer * strip + blockOfStrip;
                
                diskNumber = 2 * (temp % (disks / 2)); //algorithm to calculate the disk we write to
                
                disk_array_write( my_disk_array, diskNumber, blockNumber, data );
                disk_array_write( my_disk_array, (diskNumber + 1), blockNumber, data );
            }
        }
        
        else if(strcmp("FAIL", commandLine[0]) == 0) { //FAIL DISK
            disk_array_fail_disk( my_disk_array, atoi(commandLine[1]));
            working_disks[atoi(commandLine[1])] = FALSE;
        }
        
        
        //
        else if (strcmp("RECOVER", commandLine[0]) == 0) { //RECOVER DISK
            disk_array_recover_disk( my_disk_array, atoi(commandLine[1]));
            
            //Personal Note: HERE or after following portion?
            working_disks[atoi(commandLine[1])] = TRUE;
            int mirror_disk;
            
            //even?
            if(atoi(commandLine[1]) % 2)
                mirror_disk = (atoi(commandLine[1]) - 1);
            else
                mirror_disk = (atoi(commandLine[1]) + 1);
            
            //Personal Note:
            if (working_disks[mirror_disk]){
                int k = 0;
                
                for(k = 0; k < size; ++k){
                    disk_array_read( my_disk_array, mirror_disk, k, data);
                    disk_array_write( my_disk_array, atoi(commandLine[1]), k, data);
                }
                
            } else {
                //debugging
                printf("Both disks had previously failed!\n");
                
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(-1);
            }
        }
        
        
        else if (strcmp("END", commandLine[0]) == 0) { // END
            disk_array_print_stats(my_disk_array);
            break;
        }
        
        else {
            //debugging
            printf("%s\n","Trace File Error");
            
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(-1);
        }
        free(command);
    }
    
}

/*
 * used in disk recovery to resturn the value of the missing data
 *
 *@param int diskNumber, int blockNumber
 *@returns char *parityValue
 */
char fromParity(int blockNumber, int dataDisk, disk_array_t my_disk_array) {
    
    char blocks[disks][1024];
    int i, j;
    for(i = 0; i < disks; i++){
        if(i != dataDisk){ // we don't want to read old datadisk data
            disk_array_read(my_disk_array, i, blockNumber, blocks[i]); //if this doesnt work change "block[i]" into "&block[i][0]"
        }
    }
    for(i = 0; i < 1024; i++){
        int parity = 0;
        for(j = 0; j < disks; j++){
            if(j == dataDisk)
                continue; //do nothing
            parity = parity ^ blocks[j][i];
        }
        blocks[dataDisk][i] = parity;
    }
    disk_array_write(my_disk_array, dataDisk, blockNumber, blocks[dataDisk]);
    //if this doesnt work change "block[parityDisk]" into "&block[parityDisk][0]"
    
    return 0;
    
}


/*
 * Uses a switch to decide between different RAID systems
 *
 *@param choice :must be a single int passed in
 *
 */
void chooseSystem(int choice) {
    
    switch (choice) {
        case 0: doRaid0(); break;
        case 5: doRaid5(); break;
        case 4: doRaid4(); break;
        default: doRaid10(); break; //case 10
    }
    
}


int main(int argc, char * argv[]) {
    
    //has appropiate amount of arguments?
    if ((argc != 11) && (argc != 12)) {
        //debugging
        printf("%s\n","Arg Error");
        
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    
    //set
    for (counter = 0; counter < argc; ++counter) {
        if (strcmp("-level", argv[counter]) == 0) {
            level = atoi(argv[counter + 1]);
            
        } else if (strcmp("-strip", argv[counter]) == 0) {
            strip = atoi(argv[counter + 1]);
            
        } else if (strcmp("-disks", argv[counter]) == 0) {
            disks = atoi(argv[counter + 1]);
            
        } else if (strcmp("-size", argv[counter]) == 0) {
            size = atoi(argv[counter + 1]);
            
        } else if (strcmp("-trace", argv[counter]) == 0) {
            trace = argv[counter + 1];
            
        } else if (strcmp("-verbose", argv[counter]) == 0) {
            verbose = 1;
        }
    }
    
    //CHECK ARGUMENT VALUES
    //valid level of RAID?
    if ((level != 0) && (level != 10) && (level != 4) && (level != 5)) {
        flag = 1;
        //debugging
        printf("%s\n","Invalid or Not Provided Level");
    }
    
    /*******************************
     COMPLETE
     CHECK
     ARGUMENT
     VALUES
     *******************************/
    
    //exit if flag was raised
    if (flag == 1) {
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    //open file
    if( (trace_file = fopen(trace, "r")) == NULL){
        //debugging
        printf("%s\n","Unable to Open Trace File");
        
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(-1);
    }
    
    
    //create disk array
    
    //start all disks at working
    int i = 0;
    working_disks = malloc(disks * sizeof(int));
    for(i = 0; i < disks; ++i) {
        working_disks[i] = TRUE;
    }
    
    chooseSystem(level);
    
    fclose(trace_file);
    
    return 0;
}
