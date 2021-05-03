import networkx as nx
import math

import networkx as nx
from itertools import chain, combinations

# This function takes as input a graph g.
# The graph is complete (i.e., each pair of distinct vertices is connected by an edge),
# undirected (i.e., the edge from u to v has the same weight as the edge from v to u),
# and has no self-loops (i.e., there are no edges from i to i).
#
# The function should return an optimal weight of a Hamiltonian cycle.

# This function returns all the subsets of the given set s in the increasing order of their sizes.
def powerset(s):
    return chain.from_iterable(combinations(s, r) for r in range(len(s) + 1))


# This function finds an optimal Hamiltonian cycle using the dynamic programming approach.
def dynamic_programming(g):
    # n is the number of vertices.
    n = g.number_of_nodes()

    # The variable power now contains a tuple for each subset of the set {1, ..., n-1}.
    power = powerset(range(1, n))
    # The variable T is a dictionary, where the element T[s, i] for a set s and an integer i
    # equals the shortest path going through each vertex from s exactly once,
    # and ending at the vertex i.
    # Note that i must be in s.
    # Also, we will always assume that we start our cycle from the vertex number 0.
    # Thus, for convenience, we will always exclude the element 0 from the set s.
    T = {}
    # For every non-zero vertex i, we say that T[ tuple with the element i only, i]
    # equals the weight of the edge from 0 to i.
    # Indeed, by the definition of T, this element must be equal to the weight of
    # the shortest path which goes through the vertices 0 and i and ends at the vertex i.
    for i in range(1, n):
        # Syntactic note: In Python, we define a tuple of length 1 that contains the element i as (i,) *with comma*.
        T[(i,), i] = g[0][i]['weight']

    # For every subset s of [1,...,n-1]
    for s in power:
        # We have already initialized the elements of T indexed by sets of size 1, so we skip them.
        if len(s) > 1:
            # For every vertex i from s which we consider as the ending vertex of a path going through vertices from s.
            for i in s:
                # Define the tuple which contains all elements from s without *the last vertex* i.
                t = tuple([x for x in s if x != i])
                # Now we compute the optimal value of a cycle which visits all vertices from s and ends at the vertex i.

                # WRITE YOUR CODE HERE
                for j in t:
                    curr = T.get((s,i), float("inf"))
                    T[s,i] = min(curr, T[t,j] + g[i][j]['weight'])

    # Return the weight of on optimal cycle - this is the minimum of the following sum:
    # weight of a path + the last edge to the vertex 0.
    return min(T[tuple(range(1, n)), i] + g[i][0]['weight'] for i in range(1, n))

# This function computes a lower bound on the length of Hamiltonian cycles starting with vertices in the list sub_cycle.
# I would recommend to first see the branch_and_bound function below, and then return to lower_bound.
def lower_bound(g, sub_cycle):
    # The weight of the current path.
    current_weight = sum([g[sub_cycle[i]][sub_cycle[i + 1]]['weight'] for i in range(len(sub_cycle) - 1)])

    # For convenience we create a new graph which only contains vertices not used by g.
    unused = [v for v in g.nodes() if v not in sub_cycle]
    h = g.subgraph(unused)
    

    # Compute the weight of a minimum spanning tree.
    t = list(nx.minimum_spanning_edges(h))
    mst_weight = sum([h.get_edge_data(e[0], e[1])['weight'] for e in t])

    # If the current sub_cycle is "trivial" (i.e., it contains no vertices or all vertices), then our lower bound is
    # just the sum of the weight of a minimum spanning tree and the current weight.
    if len(sub_cycle) == 0 or len(sub_cycle) == g.number_of_nodes():
        return mst_weight + current_weight

    # If the current sub_cycle is not trivial, then we can also add the weight of two edges connecting the vertices
    # from sub_cycle and the remaining part of the graph.
    # s is the first vertex of the sub_cycle
    s = sub_cycle[0]
    # t is the last vertex of the sub_cycle
    t = sub_cycle[-1]
    # The minimum weight of an edge connecting a vertex from outside of sub_sycle to s.
    min_to_s_weight = min([g[v][s]['weight'] for v in g.nodes() if v not in sub_cycle])
    # The minimum weight of an edge connecting the vertex t to a vertex from outside of sub_cycle.
    min_from_t_weight = min([g[t][v]['weight'] for v in g.nodes() if v not in sub_cycle])

    # Any cycle which starts with sub_cycle must be of length:
    # the weight of the edges from sub_cycle +
    # the minimum weight of an edge connecting sub_cycle and the remaining vertices +
    # the minimum weight of a spanning tree on the remaining vertices +
    # the minimum weight of an edge connecting the remaining vertices to sub_cycle.
    return current_weight + min_from_t_weight + mst_weight + min_to_s_weight


# The branch and bound procedure takes
# 1. a graph g;
# 2. the current sub_cycle, i.e. several first vertices of cycle under consideration.
# Initially sub_cycle is empty;
# 3. currently best solution current_min, so that we don't even consider paths of greater weight.
# Initially the min weight is infinite
def branch_and_bound(g, sub_cycle=None, current_min=float("inf")):
    # If the current path is empty, then we can safely assume that it starts with the vertex 0.
    if sub_cycle is None:
        sub_cycle = [0]

    # If we already have all vertices in the cycle, then we just compute the weight of this cycle and return it.
    if len(sub_cycle) == g.number_of_nodes():
        weight = sum([g[sub_cycle[i]][sub_cycle[i + 1]]['weight'] for i in range(len(sub_cycle) - 1)])
        weight = weight + g[sub_cycle[-1]][sub_cycle[0]]['weight']
        return weight

    # Now we look at all nodes which aren't yet used in sub_cycle.
    unused_nodes = list()
    for v in g.nodes():
        if v not in sub_cycle:
            unused_nodes.append((g[sub_cycle[-1]][v]['weight'], v))

    # We sort them by the distance from the "current node" -- the last node in sub_cycle.
    unused_nodes = sorted(unused_nodes)

    for (d, v) in unused_nodes:
        assert v not in sub_cycle
        extended_subcycle = list(sub_cycle)
        extended_subcycle.append(v)
        # For each unused vertex, we check if there is any chance to find a shorter cycle if we add it now.
        if lower_bound(g, extended_subcycle) < current_min:
            #print(len(unused_nodes))
            new_min = branch_and_bound(g,extended_subcycle, current_min)
            if (new_min > current_min):
                return  float("inf")
            current_min = new_min

 #           if len(unused_nodes) == 1:
            print(str(extended_subcycle)+" current_min is now " + str(current_min))
            # WRITE YOUR CODE HERE
            # If there is such a chance, we add the vertex to the current cycle, and proceed recursively.
            # If we found a short cycle, then we update the current_min value.


    # The procedure returns the shortest cycle length.
    #print("returning  " + str(current_min))
    return current_min

# This function computes the distance between two points.
def dist(x1, y1, x2, y2):
    return math.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2)

# This function receives a list of 2-tuples representing the points' coordinates,
# and returns the corresponding graph.
def get_graph(coordinates):
    g = nx.Graph()
    n = len(coordinates)
    for i in range(n):
        for j in range(i + 1):
            g.add_edge(i, j, weight=dist(coordinates[i][0], coordinates[i][1], coordinates[j][0], coordinates[j][1]))
    return g

def approximation(g):
    # n is the number of vertices.
    n = g.number_of_nodes()

    # Calculate the Minimum Spanning Tree of the graph g
    st = nx.minimum_spanning_tree(g)
    
    # Get a list of vertices of the given graph in depth-first preorder.
    preorder = list(nx.dfs_preorder_nodes(st, 0))

    w = sum([g[preorder[i]][preorder[i + 1]]['weight'] for i in range(len(preorder) - 1)])
    w = w + g[preorder[-1]][preorder[0]]['weight']
    
    return w

#[(174, 25), (129, 99), (268, 212), (211, 209), (156, 82)]
#the output is inf
#while the correct answer is 495.2566051488006
coordinates = [(174, 25), (129, 99), (268, 212), (211, 209), (156, 82)]
# Create a corresponding graph.
g = get_graph(coordinates)
print("branch_and_bound:" + str(branch_and_bound(g)))

print("dp:"+str(dynamic_programming(g)))
print()
coordinates = [(0, 0), (0, 1), (0, 3), (0, 6)]
# Create a corresponding graph.
g = get_graph(coordinates)
print("branch_and_bound:" + str(branch_and_bound(g)))
print("dp:"+str(dynamic_programming(g)))
print("approximation" + str (approximation(g)))
print()

coordinates = [(0, 0), (1, 1)]
# Create a corresponding graph.
g = get_graph(coordinates)
print("branch_and_bound:" + str(branch_and_bound(g)))
print("dp:"+str(dynamic_programming(g)))
print("approximation" + str (approximation(g)))
print()

coordinates = [(231, 91), (7, 21), (226, 276), (11, 266)]
# Create a corresponding graph.
g = get_graph(coordinates)
print("branch_and_bound:" + str(branch_and_bound(g)))
print("dp:"+str(dynamic_programming(g)))
print("approximation" + str (approximation(g)))
print()

coordinates = [(178, 212), (287, 131), (98, 156)]
# Create a corresponding graph.
g = get_graph(coordinates)
print("branch_and_bound:" + str(branch_and_bound(g)))
print("dp:"+str(dynamic_programming(g)))
print("approximation" + str (approximation(g)))
print()

