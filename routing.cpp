#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <algorithm>    /// max
#include <iostream>
#include <limits.h>

using namespace std;

#define MAX_ROWCOL      100
#define MAX_EVENTS      16384
#define CALL_ARRIVAL    1
#define CALL_END        0
#define INF             999

int propdelay[MAX_ROWCOL][MAX_ROWCOL];              /// Stores all delays
int capacity[MAX_ROWCOL][MAX_ROWCOL];
int available[MAX_ROWCOL][MAX_ROWCOL];
float graph[MAX_ROWCOL][MAX_ROWCOL];

float avgHops, avgDelay;
int blocked, success;

struct Event
{
    int event_type;                 /// See if event is connected, incoming, or Blocked/Done
    float strt_time;                /// Call start
    float duration;                 /// How long the call will last in min
    char source;                    /// Source node
    char dest;                      /// Destination node
    string route;         /// Route used to connect
} EventList[MAX_EVENTS];

int minDistance(float dist[], bool sptSet[], int V);

string getPath(int parent[], int j);

string djikstra(char src, char dst, int numV);

void eventShift(int event_i, int numCalls);

void processPath(string &path, int allocate);



int main(int argc, char const *argv[])
{
    /// Read topology file
    FILE *file_ptr;
    int numCalls = 0;             /// Number of events in call workload file
    float avgHop, avgDelay;        /// Average hop and delay,respectively, for all successessessful calls
    int numSucc, numBloc;
    int numNode = 0;
    string routing;
    
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

    int algo = atoi(argv[3]);

    /// Reading topology file
    string line;
    char src, dst;
    int delay,cap;
    char row, col;

    while (4 == fscanf(file_ptr, "%c %c %d %d\n", &src, &dst, &delay, &cap))  ///"%[^\n]\n" Read until the first \n is seen
    {
         row = src - 'A';
         col = dst - 'A';

         numNode = max((int)row, max(numNode, (int)col));

         propdelay[row][col] = delay;

         capacity[row][col] = cap;

         available[row][col] = cap;  
         available[col][row] = -1;

        /// Have ot change depending on protocol
        switch (algo)
        {
        case 1:
            graph[row][col] = 1;
            graph[col][row] = 1;
            break;
        case 2:
            graph[row][col] = delay;
            graph[col][row] = delay;
            break;
        case 3:                    /// DELETE Later
            graph[row][col] = 1;
            graph[col][row] = 1;
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

    while( fscanf(file_ptr, "%f %c %c %f", &strt, &src, &dst, &duration) == 4 )
    {
        EventList[numCalls].event_type = CALL_ARRIVAL;
        EventList[numCalls].strt_time = strt;
        EventList[numCalls].duration = duration;
        EventList[numCalls].source = src;
        EventList[numCalls].dest = dst;
        numCalls++;
    }
    fclose(file_ptr);


    avgDelay = 0;
    avgHop = 0;
    success = 0;
    blocked = 0;

    i = 0;
    int handledCalls = 0;

    // EventList[1].strt_time += EventList[1].duration;
    // eventShift(1);

    // string temp = shpf('B', 'C', numNode+1);
    // printf("%s\n", temp.c_str());

    while (handledCalls != numCalls)
    {
        /// Read event
        int type = EventList[i].event_type;
        if ( type == 1 )
        {
            string res = djikstra(EventList[i].source, EventList[i].dest, numNode+1);
            printf("Main while: %c -> %c = %s\n", EventList[i].source, EventList[i].dest, res.c_str());
            if ( res.length() > 0 )
            {                
                success++;
                EventList[i].strt_time += EventList[i].duration;
                EventList[i].event_type = CALL_END;
                EventList[i].route = res;
                processPath(res, 1);
                eventShift(i, numCalls);
            } else 
            {
                blocked++;
                EventList[i].event_type = 0;
                i++;
            }
        }
        else
        {
            processPath(EventList[i].route, 0);
            EventList[i].event_type = 0;
            i++;
            handledCalls++;
        }
    }

    // for(int k=0; k < numCalls; k++)
    // {
    //     printf("%1.6f %c %c %d\n", EventList[k].strt_time, EventList[k].source, EventList[k].dest, EventList[k].event_type);
    // }
    cout << success << " " <<  blocked << endl;
     
    return 0;
}


void eventShift(int event_i, int numCalls)
{
    float strt, duration;
    int type;
    int head = event_i + 1;
    int tail = event_i;
    char src, dest;
    string rt;

    /// Get copy
    strt = EventList[event_i].strt_time;
    duration = EventList[event_i].duration;
    type = EventList[event_i].event_type;
    src = EventList[event_i].source;
    dest = EventList[event_i].dest;
    rt = EventList[event_i].route;


    while (1)
    {
        if( EventList[head].strt_time >= strt  || head == numCalls)
        {
            EventList[tail].strt_time = strt;
            EventList[tail].duration = duration;
            EventList[tail].event_type = type;
            EventList[tail].source = src;
            EventList[tail].dest = dest;
            EventList[tail].route = rt;
            break;
        }
        else
        {
            EventList[tail].strt_time = EventList[head].strt_time;
            EventList[tail].duration = EventList[head].duration;
            EventList[tail].event_type = EventList[head].event_type;
            EventList[tail].source = EventList[head].source;
            EventList[tail].dest = EventList[head].dest;
            EventList[tail].route = EventList[head].route;
        }
        tail = head;
        head++;
    }

}


void processPath(string &path, int allocate)
{
    float callDelay = 0;

    if ( allocate )
        avgHops += ((float)path.length() - 1 );

    for (int i = 1; i < path.size(); i++)
    {
        char row = ((char)path[i - 1]) - 'A';
        char col = ((char)path[i]) - 'A';

        if ( available[row][col] == -1 ) 
        {
            char temp = row;
            row = col;
            col =  temp;
        }

        if ( !allocate ) available[row][col]++;
        else
        {
            available[row][col]--;
            callDelay += propdelay[row][col];
        }

    }

    if ( allocate )
        avgDelay += callDelay;   
}


string djikstra(char src, char dst, int numV) 
{
    int src_i = (int)(src - 'A');
    int dst_i = (int)(dst - 'A');

    float dist[numV];
    bool sptSet[numV];
    int parent[numV];

    parent[src_i] = -1;
    for(int i = 0; i < numV; i++)
    {
        dist[i] = INF;
        sptSet[i] = false;
    }


    dist[src_i] = 0;

    for(int step = 0; step < numV - 1; step++)
    {
        int u = minDistance(dist, sptSet, numV);
        sptSet[u] = true;


		for (int v = 0; v < numV; v++)
        {
			if (!sptSet[v] && graph[u][v] && dist[u] + graph[u][v] < dist[v]
            && ( available[u][v] > 0 || available[v][u] > 0 ) ) 
			{ 
				parent[v] = u; 
				dist[v] = dist[u] + graph[u][v]; 
			} 
        } 
    }

    if (dist[dst_i] == INF) return "";


    string path;
    path += src;
    path += getPath(parent, dst_i);

    return path;
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


int minDistance(float dist[], bool sptSet[], int V) 
{ 
	// Initialize min value 
	float min = INF;
    int min_index; 

	for (int v = 0; v < V; v++) 
		if (sptSet[v] == false && 
				dist[v] <= min) 
			min = dist[v], min_index = v; 

	return min_index; 
} 



