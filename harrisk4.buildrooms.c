/* Kristen Harrison
344, Program 2: Adventure
compile with gcc -o harrisk4.buildrooms harrisk4.buildrooms.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>




/* Each room has a name, between 3 and 6 connections 
(the number of outgoing  connections is random), and a room type. 
*/
struct room
{
	char* name;
	char* type;
	int numConnections;
	struct room* outboundConnections[6];
};



/* allocates space for a room struct, sets the type from a parameter,
and picks a random name that has not yet been used -- used/unused name 
status is tracked in a 0-1 boolean array */
struct room* createRoom(char* names[], int namesUsed[], char* type)
{
	struct room *newRoom = malloc(sizeof(struct room));
	newRoom->type = type;
	newRoom->numConnections = 0;

	/* pick random number between 0 and 9 and check that the name at 
	that index has not yet been used */
	int randIndex;
	do {
		randIndex = rand() % 10;
	} while (namesUsed[randIndex] != 0);

	/* assign unused name to char pointer and mark as used */	
	newRoom->name = names[randIndex];
	namesUsed[randIndex] = 1;

	/* initialize connections array to null pointers */
	int j;
	for (j = 0; j < 6; j++){
		newRoom->outboundConnections[j] = 0;
	}

	return newRoom;
}



/* returns 1 if all rooms have at least 3 connections; 0 otherwise 
*/
int isGraphFull(struct room* roomsArray[])
{
	int i; 

	/* return 0 if any room has less than 3 connections */
	for (i = 0; i < 7; i++){
		if (roomsArray[i]->numConnections < 3){
			return 0;
		}
	}
	return 1;
}



/* return 1 if the input struct has fewer than 6 connections; 0 otherwise
*/
int canAddConnectionFrom(struct room* x)
{
	/* a connection is possible if the room has fewer than 6 */
	if (x->numConnections < 6){
		return 1;
	/* otherwise no more connections can be made */
	} else {
		return 0;
	}
}



/* return 1 if the two struct pointers point to the same struct; 0 otherwise
*/
int isSameRoom(struct room* x, struct room* y)
{
	if (x == y){
		return 1;
	} else {
		return 0;
	}
}



/* return 1 if the rooms already connect to each other; 0 otherwise 
*/
int connectionAlreadyExists(struct room* x, struct room* y)
{
	/* check if any x connections point to same location as y */
	int i;
	for (i = 0; i < x->numConnections; i++){
		if (x->outboundConnections[i] == y){
			return 1;
		}
	}
	/* otherwise they are not connected */
	return 0;
}



/* connect the two rooms to each other 
*/
void connectRooms(struct room* x, struct room* y)
{
	x->outboundConnections[x->numConnections] = y;
	y->outboundConnections[y->numConnections] = x;

	x->numConnections++;
	y->numConnections++;
}



/* returns a random struct room pointer 
*/
struct room* getRandomRoom(struct room* roomsArray[])
{
	/* pick room by index using random number between 0 and 6 */
	int randIndex = rand() % 7;
	struct room* randRoom = roomsArray[randIndex];
	return randRoom;
}



/* picks two random valid rooms and connects them 
*/
void addRandomConnection(struct room* roomsArray[])
{

	struct room* A;
	struct room* B;

	/* pick a room that has fewer than 6 connections */
	do {
		A = getRandomRoom(roomsArray);
	} while (canAddConnectionFrom(A) == 0);

	/* pick a room that can connect to A */
	do {
		B = getRandomRoom(roomsArray);
	} while (canAddConnectionFrom(B) == 0 || isSameRoom(A, B) == 1
		|| connectionAlreadyExists(A, B) == 1);

	/* both rooms are valid for the connection, so connect them */
	connectRooms(A, B);
}



int main(void)
{
	/* seed random number generator */
	srand(time(0));

	/* hardcode ten room names and three room types */
	char* names[10] = { "POTATO", "JUICY", "DROOPY", "BARNACLE", 
	"WEEVIL", "TUBULAR", "DANK", "BLORBY", "EISBAER", "PARFAIT" }; 

	char* types[3] = {"START_ROOM", "MID_ROOM", "END_ROOM" }; 


	/* keep track with booleans of which names have been used */
	int namesUsed[10] = {0};

	/* get pid and convert to string */
	int pid = getpid();
	char pid_str[8];
	sprintf(pid_str, "%i", pid);

	/* build directory name with process id appended */
	char dir_name[50];
	memset(dir_name, '\0', 50);
	strcpy(dir_name, "harrisk4.rooms.");
	strcat(dir_name, pid_str);
 
	/* create directory and check for errors */
	int dir_result = mkdir(dir_name, 0755); 
	if (dir_result != 0){
		printf("The file directory could not be created.");
		return 1;
	}

	
	/* create seven rooms with randomly assigned names; 
	Type 0 is start_room, 1 is mid_room, 2 is end_room */
	struct room* roomsArray[7];
	roomsArray[0] = createRoom(names, namesUsed, types[0]);
	roomsArray[1] = createRoom(names, namesUsed, types[1]);
	roomsArray[2] = createRoom(names, namesUsed, types[1]);
	roomsArray[3] = createRoom(names, namesUsed, types[1]);
	roomsArray[4] = createRoom(names, namesUsed, types[1]);
	roomsArray[5] = createRoom(names, namesUsed, types[1]);
	roomsArray[6] = createRoom(names, namesUsed, types[2]);	
	
	/* add connections until requirements are satisfied */
	while (isGraphFull(roomsArray) == 0){
		addRandomConnection(roomsArray); 
	}

	/* loop through roomsArray and create a file for each room */
	int i;
	for (i = 0; i < 7; i++){
		/* build file path from directory and room name */
		char room_filename[100];
		memset(room_filename, '\0', 100);
		sprintf(room_filename, "%s/%s_%s", dir_name, roomsArray[i]->name, "rm");

		/* create file, open, write data, close */
		FILE *fp = fopen(room_filename, "w");
		if (fp != NULL)
		{
		    fprintf(fp, "ROOM NAME: %s\n", roomsArray[i]->name);
			int j;
			for (j = 0; j < roomsArray[i]->numConnections; j++){
				fprintf(fp, "CONNECTION %i: %s\n", j+1, roomsArray[i]->outboundConnections[j]->name);	
			}
			fprintf(fp, "ROOM TYPE: %s\n\n", roomsArray[i]->type);
			fclose(fp);
		} else {
	    	perror("the file did not correctly open");		
		}
	}

	/* free room structs */	
	for (i = 0; i < 7; i++){
		int j;
		for (j = 0; j < 6; j++){
			roomsArray[i]->outboundConnections[j] = 0;
		}
		free(roomsArray[i]);
	} 

	return 0;
}