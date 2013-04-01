
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[]){

  char* error_msg = "Error\n";
	char* opt_list  = "level:strip:disks:size:trace:verbose";
	int counter     = 0;
	int level       = -1;
	int strip       = -1;
	int disks       = -1;
	int size        = -1;
	char* trace     = NULL;
	int verbose     = 0;
	int exit_flag   = 0;

	//has appropiate amount of arguments?
	if((argc != 11) || (argc != 13)){
		write(STDERR_FILENO, error_msg, strlen(error_msg));
		exit(-1);
	}

	//set
	for(counter = 0; counter < (argc - 1)/2; ++counter){
		if(strcmp("-level", argv[2*counter+1]) == 0){
			level = atoi(argv[2*counter+2]);

		}else if(strcmp("-strip", argv[2*counter+1]) == 0){
			strip = atoi(argv[2*counter+2]);

		}else if(strcmp("-disks", argv[2*counter+1]) == 0){
			disks = atoi(argv[2*counter+2]);

		}else if(strcmp("-size", argv[2*counter+1]) == 0){
			size = atoi(argv[2*counter+2]);

		}else if(strcmp("-trace", argv[2*counter+1]) == 0){
			trace = argv[2*counter+å2];
		
		}else if(strcmp("-verbose", argv[2*counter+1]) == 0){
			verbose = argv[2*counter+2];
		}
	}

	//CHECK ARGUMENT VALUES
	//valid level of RAID?
	if((level != 0) && (level != 10) && (level != 4) && (level != 5)){
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
	if(flag == 1){
		write(STDERR_FILENO, error_msg, strlen(error_msg));
		exit(-1);
	}

	
	//create disk array
	disk_array_t my_disk_array = disk_array_create("MyVirtualDiskArray", disks, size);
	if(!my_disk_array){
		//debugging
		printf("%s\n","Unable to Create Virtual Disks");
		
		write(STDERR_FILENO, error_msg, strlen(error_msg));		
		exit(-1);
	}

}
