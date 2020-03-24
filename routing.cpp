/***********************************************************************
 * Last Name:   Flores
 * First Name:  Justin
 * Student ID:  30045372
 * Course:      CPSC 441
 * Tutorial:    T05
 * Assignment:  3
 * File name: scan.cpp
 * 
 * Program that simulates routing algorithms and
 * priting out metrics that indicate the performance
 * of the routing algorithm
 * 
 * To compile:
 *  g++ routing.cpp -o routing
 * To run:
 *  ./routing <topology file> <call workload file> <routing algorithm #>
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <algorithm>    /// max


using namespace std;

#define MAX_ROWCOL      100                         /// Dimmension of cost matrix
#define MAX_EVENTS      16384                       /// Max number of events that the algorithm can handle
#define CALL_ARRIVAL    1                           /// Code for incoming call
#define CALL_END        0                           /// Code for ending call
#define INF             999                         /// How infinity is represented for the cost graphs

int propdelay[MAX_ROWCOL][MAX_ROWCOL];              /// Link delays
int capacity[MAX_ROWCOL][MAX_ROWCOL];               /// Link capacity
int available[MAX_ROWCOL][MAX_ROWCOL];              /// Current available connection in each link
float graph[MAX_ROWCOL][MAX_ROWCOL];                /// Djikstra cost graph

float avgHop, avgDelay;                             /// Hop and Delay metrics
int blocked, success;                               /// Number of successful and blocked calls


struct Event                        /// Event object
{
    int event_type;                 /// See if event is connected, incoming, or Blocked/Done
    float strt_time;                /// Call start
    float duration;                 /// How long the call will last in min
    char source;                    /// Source node
    char dest;                      /// Destination node
    string route;                   /// Route used to connect
} EventList[MAX_EVENTS];            /// Number of events to handle

/**
 * Find the node the "lowest" cost node that has not been evaluated
 * 
 * @param dist cost from src to all nodes
 * @param sptSet indicates if a node has been evaluated
 * @param V number of node
 * @param rt routing algorithm used
 * 
 * @return tne index of the next node to be evaluated
 **/
int minDistance(float dist[], bool sptSet[], int V, int rt);

/**
 * Converts the int representation of path
 * to string representation
 * 
 * @param parent array containing parent of each node
 * @param j index number of node
 * 
 * @return the string format of the path to be taken
 **/
string getPath(int parent[], int j);


/**
 *  Find the most "optimize" path from source node
 *  to destination node
 * 
 *  @param src source node
 *  @param dst destination node
 *  @param numV number of nodes in the graph
 *  @param rtr determines which evaluation method to use
 * 
 *  @return string representaion of optimized path
 **/
string djikstra(char src, char dst, int numV, int rt);

/**
 * Shift event index in EventList to keep 
 * the list in an ascending order based on
 * event start time
 * 
 * @param event_i index of event
 * @param numCalls number of events in EventList
 **/
void eventShift(int event_i, int numCalls);

/**
 * A allocate or deallocate network resource 
 * using a given path
 * 
 * @param path path to follow when allocating and deallocating resource
 * @param allocate 1 for allocate and 0 for deallocated
 * @param rta routing algorithm used
 **/
void processPath(string &path, int allocate, int rta);

/// Driver code for the program
int main(int argc, char const *argv[])
{
    /// Read topology file
    FILE *file_ptr;               /// FD for file to be read
    int numCalls = 0;             /// Number of events in call workload file
    int numNode = 0;              /// Number of nodes in network
    
    if ( argc != 4)
    {
        printf("Usage: %s <topology file> <call workload file> <routing alogrithm>\n", argv[0]);
        exit(0);
    }

    /// Initialize the graph table with INF/0 values
    int i;
    int j;
    for(i = 0; i < 26; i++)
    {
        for(j = 0; j < 26 + 1; j++)
        {
            graph[i][j] = INF;
        }
    }

    char *topology_file = (char *)argv[1];
    file_ptr = fopen(topology_file, "r");
    if ( file_ptr == NULL)
    {
        printf("Error! Could not open %s. Aborting program...\n", topology_file);
        exit(0);
    }

    int rt_algo = atoi(argv[3]);

    /// Reading topology file
    char src, dst;
    int delay,cap;
    char row, col;

    while (4 == fscanf(file_ptr, "%c %c %d %d\n", &src, &dst, &delay, &cap))  ///"%[^\n]\n" Read until the first \n is seen
    {
         row = src - 'A';
         col = dst - 'A';

         numNode = max((int)row, max(numNode, (int)col));

        /// Initializing link information and state
         propdelay[row][col] = delay;
         capacity[row][col] = cap;
         available[row][col] = cap;  
         available[col][row] = -1;              /// Indicates that the capacity of link can be found in available[row][col]

        /// Initializing cost matrx based on routing algorithm used
        switch (rt_algo)
        {
        case 1:
        case 5:
            graph[row][col] = 1;
            graph[col][row] = 1;
            break;
        case 2:
            graph[row][col] = delay;
            graph[col][row] = delay;
            break;

        case 3:
            graph[row][col] = -1;
            graph[col][row] = -1;
            break;

        case 4:                 
            graph[row][col] = -((float)cap);
            graph[col][row] = -((float)cap);
            break;
        }
    }
    fclose(file_ptr);

    
  
    char *workload_file = (char *)argv[2];
    file_ptr = fopen(workload_file, "r");

    if ( file_ptr == NULL)
    {
        printf("Error! Could not open %s. Aborting program...\n", workload_file);
        exit(0);
    }

    float strt, duration;

    /// Reading call work load files
    while( fscanf(file_ptr, "%f %c %c %f", &strt, &src, &dst, &duration) == 4 )
    {
        /// Initialize event information
        EventList[numCalls].event_type = CALL_ARRIVAL;
        EventList[numCalls].strt_time = strt;
        EventList[numCalls].duration = duration;
        EventList[numCalls].source = src;
        EventList[numCalls].dest = dst;
        numCalls++;
    }
    fclose(file_ptr);

    /// Initializing simulation metrics
    avgDelay = 0;
    avgHop = 0;
    success = 0;
    blocked = 0;

    i = 0;
    int handledCalls = 0;

    /// Processing calls in event list
    while (handledCalls != numCalls)
    {
        /// Read event
        int type = EventList[i].event_type;
        if ( type == 1 )                        /// Case when call is an incoming call
        {

            int rt = rt_algo > 2 ? 1 : 0;

            string res;
            res = djikstra(EventList[i].source, EventList[i].dest, numNode+1, rt);

            string empty;
            if (rt_algo == 5)
            {
                empty = djikstra(EventList[i].source, EventList[i].dest, numNode+1, rt);
                if ( res.size() > empty.size() ) res = "";
            }

    
            if ( res.length() > 0 )                     /// Case when call is successful
            {   
                // printf("Main while: Allocating %c -> %c = %s\n", EventList[i].source, EventList[i].dest, res.c_str());             
                success++;
                EventList[i].strt_time += EventList[i].duration;            /// Set end time for call
                EventList[i].event_type = CALL_END;                         /// Change type of event to end call
                EventList[i].route = res;                      
                processPath(res, 1, rt_algo);                               /// Allocate memory for call
                eventShift(i, numCalls);                                    /// Sort EventList 
            } else                                    /// Case when call is blocked         
            {
                blocked++;
                EventList[i].event_type = 0;
                handledCalls++;
                i++;
            }
        }
        else                    /// Case when anevent is an end call event 
        {
            // printf("Main while: Deallocating %c -> %c = %s\n", EventList[i].source, EventList[i].dest, EventList[i].route.c_str());             
            processPath(EventList[i].route, 0, rt_algo);                     /// Deallocate memory
            EventList[i].event_type = 0;
            i++;
            handledCalls++;
        }
    }

    
    /// Calculating and outputting metrics
    avgDelay /= (float)success;
    avgHop /=   (float)success;
    float blockPercentage = (100 * (float)blocked) / (float)numCalls;

    string routing;
    switch (rt_algo)
    {
    case 1:
        routing = "SHPF";
        break;
    
    case 2:
        routing = "SDPF";
        break;
    
    case 3:
        routing = "LLP";
        break;

    case 4:
        routing="MFC";
        break;
    
    case 5:
        routing="SHPO";
        break;
    }

    printf("Policy \t Calls \t Succ \t Block(%%) \t Hops \t Delay\n");
    printf("---------------------------------------------------------\n");
    printf("%4s \t %d \t % d \t %d (%.2f%%) \t %.3f \t %.3f\n", 
            routing.c_str(), numCalls, success, blocked, blockPercentage, avgHop, avgDelay);
     
    return 0;
}


void eventShift(int event_i, int numCalls)
{
    float strt, duration;
    int type;
    char src, dest;
    string rt;

    int head = event_i + 1;         /// Next call
    int tail = event_i;             /// Current call

    /// Get copy
    strt = EventList[event_i].strt_time;
    duration = EventList[event_i].duration;
    type = EventList[event_i].event_type;
    src = EventList[event_i].source;
    dest = EventList[event_i].dest;
    rt = EventList[event_i].route;


    while (1)
    {
        if( EventList[head].strt_time >= strt  || head == numCalls)             /// Case when the new index of event is found
        {
            EventList[tail].strt_time = strt;
            EventList[tail].duration = duration;
            EventList[tail].event_type = type;
            EventList[tail].source = src;
            EventList[tail].dest = dest;
            EventList[tail].route = rt;
            break;
        }
        else                                                                    /// Shifting other events
        {
            EventList[tail].strt_time = EventList[head].strt_time;
            EventList[tail].duration = EventList[head].duration;
            EventList[tail].event_type = EventList[head].event_type;
            EventList[tail].source = EventList[head].source;
            EventList[tail].dest = EventList[head].dest;
            EventList[tail].route = EventList[head].route;
        }

        /// Move window
        tail = head;
        head++;
    }

}


void processPath(string &path, int allocate, int rta)
{
    float callDelay = 0;

    /// Increment total path hops to avgHop if allocating resources
    if ( allocate )
        avgHop += ((float)path.length() - 1 );

    /// Iterate through path and allocate/deallocate network resource
    for (int i = 1; i < path.size(); i++)
    {
        char row = ((char)path[i - 1]) - 'A';       /// current node
        char col = ((char)path[i]) - 'A';           /// next node

        /// Determine where availability of link is in availability matrix
        if ( available[row][col] == -1 ) 
        {
            char temp = row;
            row = col;
            col =  temp;
        }

        if ( !allocate )                /// Case when deallocating resource
        {
            available[row][col]++;      /// Increase available connections in link

            /// Edit cost matrix when using MFC
            if ( rta == 4 ) 
            {
                graph[row][col] -= 1.0;
                graph[col][row] -= 1.0;
            }
        }
        else
        {
            available[row][col]--;  /// Decrease available connections in link

             /// Edit cost matrix when using MFC
            if ( rta == 4 ) 
            {
                graph[row][col] += 1.0;
                graph[col][row] += 1.0;
            }
            callDelay += propdelay[row][col];
        }

        /// Edit cost matrix when using LLP
        if ( rta == 3 )
        {
            float av = (float)available[row][col];
            float cap = (float)capacity[row][col];
            float new_val = av / cap;

            graph[row][col] = -new_val;
            graph[col][row] = -new_val;
        }

    }

     /// Increment total path delay to avgDelay if allocating resources
    if ( allocate )
        avgDelay += callDelay;   
}


string djikstra(char src, char dst, int numV, int rt) 
{

    /// Normalize source and destination node
    int src_i = (int)(src - 'A');
    int dst_i = (int)(dst - 'A');

    float dist[numV];           /// Cost from src to node i
    bool sptSet[numV];          /// Check if node i has already been evaluated
    int parent[numV];           /// Parent of node i

    parent[src_i] = -1;         /// Parent of source node is NULL

    /// Initialize distance to all cost to nodes to INF
    for(int i = 0; i < numV; i++)
    {
        dist[i] = INF;
        sptSet[i] = false;
    }

    dist[src_i] = rt == 1 ? -INF : 0;           /// Distance to src node is always 0

    /// Find all the "optimize" path from src node to all nodes
    for(int step = 0; step < numV - 1; step++)
    {

        int u = minDistance(dist, sptSet, numV, rt);     /// Find the "least" cost node that has not been evaluated
        
        sptSet[u] = true;       /// Mark node as evaluated
        

        /// Check if current node to other nodes is cheaper than currently computed value to those nodes
		for (int v = 0; v < numV; v++)
        {
            /// Change evaluation method depending on algorithm used
            switch (rt)
            {
            case 0:
                if (!sptSet[v] && graph[u][v] && dist[u] + graph[u][v] < dist[v]
                && ( available[u][v] > 0 || available[v][u] > 0 ) ) 
                { 
                    parent[v] = u; 
                    dist[v] = dist[u] + graph[u][v]; 
                } 
                break;
            
            case 1:
                if (!sptSet[v] && graph[u][v] && max(dist[u] , graph[u][v]) < dist[v]
                && ( available[u][v] > 0 || available[v][u] > 0 ) ) 
                { 
                    parent[v] = u; 
                    dist[v] = max(dist[u] , graph[u][v]); 
                } 
                break;
            }

        } 
    }

    if (dist[dst_i] == INF) return "";      /// When no path to destination from 

    /// Find strint representation of path
    string path;
    path += src;
    path += getPath(parent, dst_i);

    return path;
}


string getPath(int parent[], int j) 
{ 

	if (parent[j] == - 1) return "";  /// Base Case : If j is source 

    string temp;
    char node = j + 'A';
	temp += getPath(parent, parent[j]); /// Recursively find path to node j
    temp += node;

    return temp; 
} 


int minDistance(float dist[], bool sptSet[], int V, int rt) 
{ 
	float min = INF; // Initialize min value 
    int min_index; 

    /// Iterate through cost list and find the "least" cost node that has not been evaluated
	for (int v = 0; v < V; v++) 
    {
        /// Use different evaliation methond depending on routing algorithm
        switch (rt)
        {
        case 0:     /// SHPF and SDPF
            if (sptSet[v] == false && dist[v] < min) 
			    min = dist[v], min_index = v; 
            break;
        
        case 1:     /// LLP and MFC
            if (sptSet[v] == false && dist[v] <= min) 
			    min = dist[v], min_index = v; 
            break;
        }

    }

	return min_index; 
} 



string ult_shortest(char src, char dst, int numV) 
{

    /// Normalize source and destination node
    int src_i = (int)(src - 'A');
    int dst_i = (int)(dst - 'A');

    float dist[numV];           /// Cost from src to node i
    bool sptSet[numV];          /// Check if node i has already been evaluated
    int parent[numV];           /// Parent of node i

    parent[src_i] = -1;         /// Parent of source node is NULL

    /// Initialize distance to all cost to nodes to INF
    for(int i = 0; i < numV; i++)
    {
        dist[i] = INF;
        sptSet[i] = false;
    }

    dist[src_i] =  0;           /// Distance to src node is always 0

    /// Find all the "optimize" path from src node to all nodes
    for(int step = 0; step < numV - 1; step++)
    {

        int u = minDistance(dist, sptSet, numV, 0);     /// Find the "least" cost node that has not been evaluated
        
        sptSet[u] = true;       /// Mark node as evaluated
        

        /// Check if current node to other nodes is cheaper than currently computed value to those nodes
		for (int v = 0; v < numV; v++)
        {

            if (!sptSet[v] && graph[u][v] && dist[u] + graph[u][v] < dist[v] ) 
            { 
                parent[v] = u; 
                dist[v] = dist[u] + graph[u][v]; 
            } 

        } 
    }

    if (dist[dst_i] == INF) return "";      /// When no path to destination from 

    /// Find strint representation of path
    string path;
    path += src;
    path += getPath(parent, dst_i);

    return path;
}