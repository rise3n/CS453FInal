#include "LTS.h"


// start local simplification process

LocalSimplification::LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme, Polyhedron* poly)
{
	this->num_remove = 0;
	this->extreme_remove = Extremes2remove(extremes, reserved_extreme);
	this->superlevelSetList.resize(num_remove);
	superlevel_set_propagation(poly);
}

LocalSimplification::~LocalSimplification()
{

}

/*
superlevel_set_propagation process as described in paper
since extreme points could be in the quad (from homework)
expand superlevel_set_propagation 

at step 1, simply take 4 vertices of the quad as neighbors
after that, follow what depicted in paper
*/
void LocalSimplification::superlevel_set_propagation(Polyhedron* poly) 
{

	Quad *q;
	Vertex *current_node, *nextnode;
	double max = -INFINITY;
	double extreme_scalar = 0;

	//first get the quad with extreme inside the quad, register 4 vertices as neighbor
	//choose the one with largest scalar and start propagation from that point.
	for (int i = 0; i < num_remove; i++) 
	{
		std::vector<Vertex*> neighbors;
		std::vector<Vertex*> visited;
		q = poly->findquad(extreme_remove[i]);
		for (int j = 0; j < 4; j++) {
			neighbors.push_back(q->verts[j]);
			if (q->verts[j]->scalar > max) {
				max = q->verts[j]->scalar;
				current_node = q->verts[j];
			}
		}
		//remove current node 
		neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), current_node), neighbors.end());


		//extreme_scalar = current_node->scalar;
		while (true) {
			current_node->findNeighbor();

			//iteratively add neighbors of current node to neighbors list
			for (int j = 0; j < current_node->num_neighbors; j++) {
				if ((std::find(neighbors.begin(), neighbors.end(), current_node->neighbors[j]) == neighbors.end()) &&
					(std::find(visited.begin(), visited.end(), current_node->neighbors[j]) == visited.end())) //if not find, which means its new node
					neighbors.push_back(current_node->neighbors[j]);
			}
			// move to largest neighbor
			nextnode = LargestNeighbor(neighbors); 
			
			//if next node scalar value is larger than the scalar value of current node, then stop propagation
			if (nextnode->scalar > current_node->scalar)
			{
				visited.push_back(current_node);
				break;
			}
			else
			{
				neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), nextnode), neighbors.end());
				visited.push_back(current_node);
				current_node = nextnode;
			}
		}
		superlevelSetList.push_back(visited);
	}
}


/*
	this function should generate new ordering g as depicted in paper
*/
std::vector<Vertex> LocalSimplification::LocalTopologicalSimplification()
{
	std::vector<Vertex> g; // reordered g 
	


	return g;
}


/*
This function return the list of extremes to be removed given points we want to reserve

****since the paper emphasize that they follow the scheme to remove local maximum only, here I skip extremes that are not local maximum
****if we want to include local minimum as well, then we need to do the process symmetrically (for propagation and reordering)
* 
*/
std::vector<CriticalPoint> LocalSimplification::Extremes2remove(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme)
{
	std::vector<CriticalPoint> removelist;
	for (int i = 0; i < extremes.size(); i++) 
	{
		if (std::find(reserved_extreme.begin(), reserved_extreme.end(), extremes[i]) != reserved_extreme.end()) //if the extreme is in reserved
			continue;
		else
		{
			if (extremes[i].type == MAXIMUM)
			{
				this->num_remove += 1;
				removelist.push_back(extremes[i]);
			}
		}
	}

	return removelist;
}

/*
this function would merge local reordering g with nodes outside of super set
*/
std::vector<Vertex> LocalSimplification::merge(Vertex** f, std::vector<Vertex> g)
{
	std::vector<Vertex> f_new;



	return f_new;
}

/*
this function returns the node with largest scalar value in given neighbors list, use this to determine the next node we would propagate to
neighbors:	all neighboring nodes of visited nodes in superlevel set propagation
*/
Vertex* LocalSimplification::LargestNeighbor(std::vector<Vertex*> neighbors) {
	
	Vertex* ptr = NULL;
	double max = -INFINITY;
	for (int i = 0; i < neighbors.size(); i++)
	{
		if (neighbors[i]->scalar > max)
		{
			max = neighbors[i]->scalar;
			ptr = neighbors[i];
		}
	}
	return ptr;
}

