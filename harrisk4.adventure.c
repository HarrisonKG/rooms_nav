/* Kristen Harrison
344, Program 2: Adventure
compile with gcc -o harrisk4.adventure harrisk4.adventure.c -lpthread
*/

/* allow getline to work */
#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <pthread.h>


/* all room files end in _rm */
const char roomSuffix[4] = "_rm";

/* set mutex as global variable */
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;


/* room struct stores connections in array of char pointers */
struct room
{
	char* name;
	char* type;
	int numConnections;
	char** outboundConnections;
};



/* allocate space for a room, assign attributes to parameters, 
and copy array of connecting room names 
*/
struct room* createRoom(char name [], char type [], int connections, char** outBounds)
{
	/* assign pointers to the name and type arrays allocated in main */
	struct room *newRoom = malloc(sizeof(struct room));
	newRoom->name = name;
	newRoom->type = type;
	newRoom->numConnections = connections; 

	/* create array of char pointers, and copy values 
	from the parameter array */
	newRoom->outboundConnections = (char **)malloc(sizeof(char*) * 6);

	int i;
	for (i = 0; i < 6; i++) {
        newRoom->outboundConnections[i] = outBounds[i];
    }

	return newRoom;
}



/* perform a stat() function call on rooms directories 
	in the same directory as the game, and open the one with 
	most recent st_mtime component of the returned stat struct. 
*/
int findNewestRoomsDir(char* newestDir, int length)
{
	/* track most recent folder modification time */
	int newestDirTime = -1;

	/* make dir prefix string */
	char targetDirPrefix[32] = "harrisk4.rooms.";

	/* look in current directory */
	DIR* dirToCheck = opendir(".");

	/* check that the directory is accessible */
	if (dirToCheck > 0){
		/* track current subdirectory and its attributes */
		struct dirent* currSubDir;
		struct stat dirData;

		/* loop through entries in the directory */
		while ((currSubDir = readdir(dirToCheck)) != NULL){
			/* returns first instance of targetDirPrefix in d_name 
			[nonnull result confirms match] and stores the name in stat struct */
			if (strstr(currSubDir->d_name, targetDirPrefix) != NULL){
        		stat(currSubDir->d_name, &dirData); 

        		/* check if subdir found has a more recent 
	        	modification time than latest tracked */
	        	if ((int)dirData.st_mtime > newestDirTime){
	        		/* if so, update subdir tracker */
					newestDirTime = (int)dirData.st_mtime;
					memset(newestDir, '\0', length);
					strcpy(newestDir, currSubDir->d_name);
		        }
        	}	
		}
	} else {
		printf("current directory couldn't be accessed.\n");
		return 1;
	}

	closedir(dirToCheck); 
	return 0;
}



/* read through files in the newest rooms directory to create rooms
and store pointers to them in the rooms array */
void buildRooms(char newestDir[], DIR* targetDir, struct room* roomsArray[])
{
	/* track current directory entry */
	struct dirent* fileInDir;

	/* set up buffer for getline */
	char* line = NULL;
	size_t lineSize = 0;

	/* initialize array index */
	int arrayIndex = 0;
	
	/* loop through all entries in rooms folder that end in _rm */
	while ((fileInDir = readdir(targetDir)) != NULL){
		if (strstr(fileInDir->d_name, roomSuffix) != NULL){

			/* save dir/room filepath into string */
			char room_filename[32];
			sprintf(room_filename, "%s/%s", newestDir, fileInDir->d_name);

			/* open file and process contents */
			FILE* fp = fopen(room_filename, "r");
			char* roomName = malloc(9 * sizeof(char));
			char* roomType = malloc(12 * sizeof(char));
			/* 6 possible connections, stored in pointers to char */
			char** outboundConnections = (char **)malloc(sizeof(char*) * 6);
			int connections = 0;

			/* loop through lines of file */
			while(getline(&line, &lineSize, fp) != -1){
				/* determine which type of line it is */

				if(strstr(line, "ROOM NAME") != NULL){
					sscanf(line, "%*s %*s %s", roomName);
				}

				/* save each connection as char* in an array of char pointers */
				if(strstr(line, "CONNECTION ") != NULL){
					char* outBound = malloc(9 * sizeof(char));
					sscanf(line, "%*s %*s %s", outBound);
					outboundConnections[connections] = outBound;
					connections++;
				}
				
				if(strstr(line, "ROOM TYPE: ") != NULL){
					sscanf(line, "%*s %*s %s", roomType);
				}
			}

			fclose(fp);

			/* create room from the input data and save in array */
			roomsArray[arrayIndex] = createRoom(roomName, roomType, connections, outboundConnections); 
			free(outboundConnections);
			arrayIndex++;
		}
	}
	/* free allocation used by getline */
	free(line);
}



/* check if the room is the end room */
int isEndRoom(struct room* currentRoom){
	if (strcmp(currentRoom->type, "END_ROOM") == 0){
		return 1;
	} else {
		return 0;
	}
}



/* start up time thread by main thread calling unlock */
void* printTime()
{
	/* immediately try to lock mutex so that the time thread joins
	a queue for the shared resource */
	pthread_mutex_lock(&myMutex);

	/* format time string using strftime */
	char timeString[1024];
	time_t t;
	struct tm* tmp;
	t = time(0);
	tmp = localtime(&t);

	if (strftime(timeString, sizeof(timeString), "%l:%M%P, %A, %B %d, %Y", tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }
	
	/* create file, open it or overwrite existing and write time string */
	FILE *fp = fopen("currentTime.txt", "w");
	fprintf(fp, "%s\n", timeString);
	fclose(fp);

	/* return control back to main thread */
	pthread_mutex_unlock(&myMutex); 

	return NULL;
}



/* returns pointer to the start room */
struct room* getStartRoom(struct room* roomsArray[])
{
	/* look through each room in the array to find start type */ 
	struct room* startRoom;
	int i;
	for (i = 0; i < 7; i++){
		if (strcmp(roomsArray[i]->type, "START_ROOM") == 0){			
			startRoom = roomsArray[i];
		}
	}
	return startRoom;
}



/* allow the user to move between rooms or check time until 
they find end room 
*/
void playGame(struct room* roomsArray[], pthread_t time_thread)
{
	/* keep track of where user is, starting from start room */
	struct room* currentRoom = getStartRoom(roomsArray); 
	
	/* set up buffer for getline */
	char* lineEntered = NULL;
	size_t bufferSize = 0;
	int numCharsEntered = 0;
	int numSteps = 0;

	/* set up char array to store user's choice of next room */
	char roomChoice[9];
	memset(roomChoice, '\0', 9);

	/* keep track of steps taken (limit of 25) */
	char** stepsArray = (char **)malloc(sizeof(char*) * 25);


	/* loop until user finds the end room */
	while (!isEndRoom(currentRoom)){ 
		printf("CURRENT LOCATION: %s\n", currentRoom->name);
		printf("POSSIBLE CONNECTIONS: ");

		/* list connections and prompt for input */
		int j;
		for (j = 0; j < currentRoom->numConnections - 1; j++){
			printf("%s, ", currentRoom->outboundConnections[j]);	
		}
		/* do last connection separately for different punctuation */
		printf("%s. \n", currentRoom->outboundConnections[j]);	
		printf("WHERE TO? >");

		/* getline includes the newline, so track number of chars entered
		and copy one less to the char array, and append null terminator */
		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
		strncpy(roomChoice, lineEntered, numCharsEntered - 1);
		roomChoice[numCharsEntered - 1] = '\0';

		/* handle time request */
		while (strcmp(roomChoice, "time") == 0){
			/* unlock mutex so time thread can access file */
			pthread_mutex_unlock(&myMutex);
			pthread_join(time_thread, NULL);
			
			/* set up buffer for getline */
			char* line = NULL;
			size_t lineSize = 0;

			/* read line in file to output */
			FILE *fp = fopen("currentTime.txt", "r");
			getline(&line, &lineSize, fp);
			printf("\n%s\n", line);
			free(line);
			fclose(fp);

			/* lock mutex again and recreate second thread */
			pthread_mutex_lock(&myMutex); 
			pthread_create(&time_thread, NULL, printTime, NULL);

			/* set up for next user choice */
			printf("WHERE TO? >");
			numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
			strncpy(roomChoice, lineEntered, numCharsEntered - 1);
			roomChoice[numCharsEntered - 1] = '\0';
		}
		printf("\n");

		/* use bool to track whether pointer has been found */
		int foundRoom = 0;
		
		/* look through current room's connections for requested room */
		int i;
		for (i = 0; i < currentRoom->numConnections; i++){
			/* entered string matches a connecting room name */
			if (strcmp(currentRoom->outboundConnections[i], roomChoice) == 0){	
				int j;
				/* find room pointer */
				for (j = 0; j < 7; j++){
					if (strcmp(roomsArray[j]->name, roomChoice) == 0){
						/* update current room and steps taken */
						currentRoom = roomsArray[j];
						stepsArray[numSteps] = roomsArray[j]->name;
						foundRoom = 1;
						numSteps++;
					}
				}
			}
		}
		if (!foundRoom){
			printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		}
	} 
	/* free allocation used by getline */
	free(lineEntered);

	/* display victory messages */
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %i STEPS.\n", numSteps);

	/* print out steps taken */
	int k;
	for (k = 0; k < numSteps; k++){
		printf("%s\n", stepsArray[k]);
	}

	free(stepsArray);
}




int main(void)
{
	/* main thread locks the mutex before using shared resource */
	pthread_mutex_lock(&myMutex);

	/* start time thread */
	pthread_t time_thread;
	pthread_create(&time_thread, NULL, printTime, NULL);

	/* store the name of the most recently modified directory 
	with the target prefix in newestDir string */
	char newestDir[256];
	memset(newestDir, '\0', 256);
	int result = findNewestRoomsDir(newestDir, sizeof(newestDir));

	/* check for errors */
	if (result > 0){
		printf("current directory couldn't be accessed.");
		return 1;
	}

	/* create file path for newest rooms folder and open it */
	char dirPath[256];
	sprintf(dirPath, "./%s", newestDir);
	DIR* targetDir = opendir(dirPath);

	/* check if rooms directory is accessible */
	if (targetDir > 0){
		/* initialize array of rooms and set up index variable */
		struct room* roomsArray[7]; 

		/* build roomsArray by reading in room structs from files */
		buildRooms(newestDir, targetDir, roomsArray);

		/* play game until the user finds end room */
		playGame(roomsArray, time_thread);

		/* free allocated space */
		int i;
		for (i = 0; i < 7; i++){
			free(roomsArray[i]->name);
			free(roomsArray[i]->type);
			int k;
			for (k = 0; k < roomsArray[i]->numConnections; k++){
				free(roomsArray[i]->outboundConnections[k]);
			}
			free(roomsArray[i]->outboundConnections);
			free(roomsArray[i]);
		}

	} else {
		printf("The rooms directory couldn't be accessed.\n");
		return 1;
	}

	closedir(targetDir); 

	/* clean up */
	pthread_mutex_destroy(&myMutex);
	pthread_cancel(time_thread);

	return 0;
}