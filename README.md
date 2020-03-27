# Network-Layer-Routing-Algorithms
**To compile***:

``` g++ routing.cpp -o routing ```

**To run**:

``` ./routing <topologly file> <call workload file> <routing algorithm #>```

**Routing Algoithms**:

| # | Routing Algorithm | Description |
| --- | ---- | ---- | 
| 1 | Shortest Hop Path First (SHPF) | Finds the shortest path from source node to destination node, where the length of the path represents the number of hops. |
| 2 | Shortest Delay Path First (SDPF) | Finds the path with the shortest total delay from src node to destination |
| 3 | Least Loaded Path (LLP) | Finds the least loaded path where the load of a candidate path the the busiest link in the path |
| 4 | Maximum Free Circuit (MFC) | Finds the path from soure node to destination node with the  most free circuit, where the number of free circuirs of a candidate path is the link with the least free circuit in the path |
| 5 | Shortest Hop Path Only | This algorithm is similar to SHPF but it rejects calls with a hop path greater than the hop count it would take to route the same call on an empty network |

**Creating your own input files**:
- Source nodes and destination nodes must be represented using capital letters
- Topology file format: `<src> <dest> <link capacity> <delay in ms>`
- Call workload format: `<call start time> <src> <dest> <call duration>`
