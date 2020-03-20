#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <algorithm>    /// max
#include <iostream>

using namespace std;

#define MAX_ROWCOL      100
#define MAX_EVENTS      10000
#define CALL_ARRIVAL    0
#define CALL_END        1
#define INF             999

int propdelay[MAX_ROWCOL][MAX_ROWCOL];              /// Stores all delays
int capacity[MAX_ROWCOL][MAX_ROWCOL];
int available[MAX_ROWCOL][MAX_ROWCOL];
int graph[MAX_ROWCOL][MAX_ROWCOL];

struct Event
{
    int event_type;                 /// See if event is connected, incoming, or Blocked/Done
    float strt_time;                /// Call start
    float duration;                 /// How long the call will last in min
    char source;                    /// Source node
    char dest;                      /// Destination node
    string route;         /// Route used to connect
} EventList[MAX_EVENTS];

int minDistance(int dist[], bool sptSet[], int V);
string getPath(int parent[], int j);
void shpf(char src, char dst, int numV);



int main(int argc, char const *argv[])
{
    /// Read topology file
    FILE *file_ptr;
    int numevents = 0;             /// Number of events in call workload file
    float avgHop, avgDelay;        /// Average hop and delay,respectively, for all successful calls
    int numSucc, numBloc;
    int maxNode = 0;
    string routing = "SHPF";
    
    if ( argc != 3)
    {
        printf("Usage: %s <topology file> <call workload file>\n", argv[0]);
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

    /// Reading topology file
    string line;
    char src, dst;
    int delay,cap;
    char row, col;

    while (4 == fscanf(file_ptr, "%c %c %d %d\n", &src, &dst, &delay, &cap))  ///"%[^\n]\n" Read until the first \n is seen
    {
         printf("%c %c %d %d\n", src, dst, delay, cap);
         row = src - 'A';
         col = dst - 'A';
         maxNode = max((int)row, max(maxNode, (int)col));

         
         propdelay[row][col] = delay;
         capacity[row][col] = cap;
         available[row][col] = cap;  

         graph[row][col] = 1;
         graph[col][row] = 1;
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

    while( fscanf(file_ptr, "%f %c %c %f", &strt, &src, &dst, &duration) == 4 )
    {
        printf("%8.6f %c %c %8.6f\n", strt, src, dst, duration);
        EventList[numevents].event_type = CALL_ARRIVAL;
        EventList[numevents].strt_time = strt;
        EventList[numevents].duration = duration;
        EventList[numevents].source = src;
        EventList[numevents].dest = dst;
        numevents++;
    }
    fclose(file_ptr);



    /// Route call
    /// Find path and then allocate resource
    /// Check if a connection needs to expire
    /// Close connection and free up resource

    /// Read call work load
    return 0;
}

string getPath(int parent[], int j) 
{ 
	// Base Case : If j is source 
	if (parent[j] == - 1) return ""; 

    string temp;
    char node = j + 'A';

	temp += getPath(parent, parent[j]);
    temp += node;

    return temp; 
} 

int minDistance(int dist[], bool sptSet[], int V) 
{ 
	
	// Initialize min value 
	int min = INF, min_index; 

	for (int v = 0; v < V; v++) 
		if (sptSet[v] == false && 
				dist[v] <= min) 
			min = dist[v], min_index = v; 

	return min_index; 
} 

void shpf(char src, char dst, int numV) 
{
    int src_i = (int)(src - 'A');
    int dst_i = (int)(dst - 'B');

    int dist[numV];
    bool sptSet[numV];
    int parent[numV];

    for(int i = 0; i < numV; i++)
    {
        parent[0] = -1;
        dist[i] = 0;
        sptSet[i] = false;
    }

    dist[src_i] = 0;

    for(int step = 0; step < numV - 1; step++)
    {
        int u = minDistance(dist, sptSet, numV);
        sptSet[u] = true;


		for (int v = 0; v < numV; v++)
        {
			if (!sptSet[v] && graph[u][v] && dist[u] + graph[u][v] < dist[v]) 
			{ 
				parent[v] = u; 
				dist[v] = dist[u] + graph[u][v]; 
			} 
        } 
    }

    string path;
    path += src;
    path += getPath(parent, dst_i);

    cout << path << endl;
}
