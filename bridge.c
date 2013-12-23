p/*
Name: Amith Ananthram

This program simulates a 1-directional bridge, and uses a combination
of locks and conditional variables to synchronize the traffic across
this bridge.

Running this code:
./bridge [NUMBER_OF_CARS_TO_SIMULATE] [MAX_LOAD]

Note that the upper bound for this is MAX_CARS (10000).

Each car sends 1 second on the bridge (a sleep built in).p

It include the additional features poutlined in the project description:
	- cars don't pass each other once on the bridge
		(the print out does not reveal their positions on the bridge, but 
		 is rather numerically order -- this feature can be observed as cars
		 leave, however -- they leave the bridge in the order they arrived)
	- no direction is starved
		(once 2 * max_load cars have gone across the bridge in any one direction,
		 it switches traffic to the other direction)

Designs:
	2 conditional variables:
		toward_hanover
		toward_norwich

	They basically functioned as green lights for each direction of traffic.
	More exlanation in the code.
*/

#include <pthread.h> 				// for threads
#include <time.h>					// for random seed
#include <stdlib.h>	
#include <stdio.h>				
#include <unistd.h>

// directions for the bridge
#define TO_NORWICH 0
#define TO_HANOVER 1

// max cars for simulation
#define MAX_CARS 10000

// taken from lecture notes
#  define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0 } } }

// global variables
int bridge[MAX_CARS+1];				// cars on the bridge
int hanover_waiting[MAX_CARS+1];	// cars waiting for hanover
int norwich_waiting[MAX_CARS+1];	// cars waiting for norwich

int amount_of_traffic;				// total # of cars to cross (argv[1])
int max_load;

int hanover_count;					// # on hanover cars on the bridge now
int norwich_count;					// # of norwich cars on the bridge now
	
int hanover_wait;					// # of hanover cars waiting for the bridge
int norwich_wait;					// # of norwich cars waiting for the bridge

int hanover_lim;					// # of hanover cars that have crossed in a row
int norwich_lim;					// # of norwich cars that have crossed in a row

pthread_mutex_t lock =  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

/*
	toward_hanover -- a green light for the hanover traffic
		(they wait for it, and it gets signalled when the bridge is
		free of norwich traffic)
	toward_norwich -- the same as the above, but for norwich
*/

pthread_cond_t toward_hanover = PTHREAD_COND_INITIALIZER; 
pthread_cond_t toward_norwich = PTHREAD_COND_INITIALIZER;


/*
 Takes an int max and returns a random number from [0, max]
*/
int genRand(int max)
{
	return rand() % (max + 1);
}

/*
 Takes the direction of the current car, and prints the status of the
 bridge in an easy to digest fashion.
*/
void printStatus(int direction)
{
	if(direction == TO_NORWICH)
		printf("\tBRIDGE (NORWICH_BOUND):\t{");
	else
		printf("\tBRIDGE (HANOVER_BOUND):\t{");

// This prints the cars on the bridge in numerical order
// -1 means that the car at that index is not on the bridge
	for(int i = 0; i < amount_of_traffic; i++)
		if(bridge[i] != -1)
			printf(" %d ", bridge[i]);

	printf("}\n\tWAITING FOR NORWICH:\t{");

// This prints the cars waiting to go to Norwich in numerical order
// -1 means that the car at that index is not waiting for Norwich
	for(int i = 0; i < amount_of_traffic; i++)
		if(norwich_waiting[i] != -1)
			printf(" %d ", norwich_waiting[i]);

	printf("}\n\tWAITING FOR HANOVER:\t{");

// This prints the cars waiting to go to Hanover in numerical order
// -1 means that the car at that index is not waiting for Hanover
	for(int i = 0; i < amount_of_traffic; i++)
		if(hanover_waiting[i] != -1)
			printf(" %d ", hanover_waiting[i]);	

	printf("}\n\n");

	fflush(NULL);
}

/*
	The main function called by each thread.  Manage entering, crossing, and
	exiting the bridge for each vehicle.

	Includes the locks, conditional variables, etc.  Exlained in more detail
	within.
*/
void *oneVehicle(void *vargp)
{
	int rc;

	int car_number = (int) vargp;
	int direction = genRand(1);

// ArriveBridge
// Here, based on the direction, the thread gets a lock.  If there are no
// cars on the bridge going the opposite direction AND there's still room
// on the bridge for another car, it waits for its conditional variable
// (basically a green light).

	rc = pthread_mutex_lock(&lock);

	if (rc)
		{
			printf("Lock failed!\n");
			exit(-1);
		}
	
	if(direction == TO_NORWICH)
	{
// Until the signal has been acquired, this vehicle is waiting.  It's saved
// in the waiting array and counted among the waiting cars.
		norwich_wait++;		
		norwich_waiting[car_number] = car_number;

// If there's no traffic going the other way on the bridge, and still room
// (under the max_load), then wait for the green light and acquire the lock.
		while(hanover_count > 0 || norwich_count >= max_load)
		{
		pthread_cond_wait(&toward_norwich, &lock);
		}

// Now that it's received the green light, it's no longer waiting.
		norwich_waiting[car_number] = -1;	
		hanover_waiting[car_number] = -1;
		norwich_wait--;

// It's on the bridge, so now it's counted in that array.
		norwich_count++;	
		bridge[car_number] = car_number;			

// And it's consecutive count is maintained to switch directions later.
		norwich_lim++;

// The state of the bridge has changed, so print its new state.
		printf("%d is on the bridge:\n", car_number);
		printStatus(TO_NORWICH);

// If there are no more norwich-bound cars OR max_load*2 norwich-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((norwich_count == 0 || norwich_lim >= (max_load*2)) && hanover_wait > 0)
			pthread_cond_signal(&toward_hanover);
		else
			pthread_cond_signal(&toward_norwich);
	}
	else
	{
// Until the signal has been acquired, this vehicle is waiting.  It's saved
// in the waiting array and counted among the waiting cars.
		hanover_wait++;	
		hanover_waiting[car_number] = car_number;	

// If there's no traffic going the other way on the bridge, and still room
// (under the max_load), then wait for the green light and acquire the lock.
		while(norwich_count > 0 || hanover_count >= max_load)
		{
		pthread_cond_wait(&toward_hanover, &lock);
		}

// Now that it's received the green light, it's no longer waiting.
		norwich_waiting[car_number] = -1;	
		hanover_waiting[car_number] = -1;	
		hanover_wait--;

// It's on the bridge, so now it's counted in that array.
		hanover_count++;
		bridge[car_number] = car_number;

// And it's consecutive count is maintained to switch directions later.
		hanover_lim++;

// The state of the bridge has changed, so print its new state.
		printf("%d is on the bridge:\n", car_number);
		printStatus(TO_HANOVER);

// If there are no more hanover-bound cars OR max_load*2 hanover-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((hanover_count == 0 || hanover_lim >= (max_load*2)) && norwich_wait > 0)
			pthread_cond_signal(&toward_norwich);
		else
			pthread_cond_signal(&toward_hanover);
	}

	rc = pthread_mutex_unlock(&lock);

	if (rc)
		{
			printf("Unlock failed!\n");
			exit(-1);
		}

// OnBridge
// Simply a sleep, to make the simulation more meaningful.	
	sleep(1);	// takes 1 seconds to get off the bridge

// ExitBridge
// Here, the threads acquire locks and simulate exiting the bridge.
// If there's still traffic going in the same direction AND it hasn't
// reached max_load*2 cars consecutively, it signals for another car
// of the same direction on.  Else, it singals for a car from the different
// direction to come on.
	rc = pthread_mutex_lock(&lock);

	if (rc)
		{
			printf("Lock failed!\n");
			exit(-1);
		}
	
	if(direction == TO_NORWICH)
	{	
// Car has left the bridge (record it in arrays)
		norwich_count--;
		bridge[car_number] = -1;	

// The opposite direction's consecutive travel meter can be reset
		hanover_lim = 0;

// If there are no more norwich-bound cars OR max_load*2 norwich-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((norwich_count == 0 || norwich_lim >= (max_load*2)) && hanover_wait > 0)
			pthread_cond_signal(&toward_hanover);
		else
			pthread_cond_signal(&toward_norwich);

// The status of the bridge has changed, so print it again.
		printf("%d is off the bridge:\n", car_number);
		printStatus(TO_NORWICH);
	}
	else
	{
// Car has left the bridge (record it in arrays)
		hanover_count--;
		bridge[car_number] = -1;		

// The opposite direction's consecutive travel meter can be reset
		norwich_lim = 0;

// If there are no more hanover-bound cars OR max_load*2 hanover-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((hanover_count == 0 || hanover_lim >= (max_load*2)) && norwich_wait > 0)
			pthread_cond_signal(&toward_norwich);
		else
			pthread_cond_signal(&toward_hanover);

// The status of the bridge has changed, so print it again.
		printf("%d is off the bridge:\n", car_number);
		printStatus(TO_HANOVER);
	}

	rc = pthread_mutex_unlock(&lock);

	if (rc)
		{
			printf("Unlock failed!\n");
			exit(-1);
		}

	return NULL;
}

int main(int argc, char *argv[])
{
	amount_of_traffic = atoi(argv[1]);
	max_load = atoi(argv[2]);

	pthread_t cars[amount_of_traffic];		// contains all the pthreads

	int i, rc;

	srand(time(NULL));

// the following lines initialize all the threads variables to zero and the
// values in the arrays to -1

	norwich_count = 0;
	norwich_wait = 0;
	norwich_lim = 0;

	hanover_count = 0;
	hanover_wait = 0;
	hanover_lim = 0;

	for(i = 0; i < amount_of_traffic; i++)
	{
		hanover_waiting[i] = -1;
		norwich_waiting[i] = -1;
		bridge[i] = -1;
	}

// this for loop creates all the threads, simply assing them the index of
// each vehicle as an argument
	for(i = 0; i < amount_of_traffic; i++)
	{
		rc = pthread_create(&cars[i],
							NULL,
							oneVehicle,
							(void *) i);

		if (rc)
		{
			printf("Creation of new thread failed!\n");
			exit(-1);
		}
	}

// waits for the threads to close!
	for(i = 0; i < amount_of_traffic; i++)
	{
		rc = pthread_join(cars[i],
						  NULL);

		if (rc)
		{
			printf("Closing a thread failed!\n");
			exit(-1);
		}
	}



	return 0;
}
