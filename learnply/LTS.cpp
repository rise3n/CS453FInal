#include "LTS.h"
#include <iostream>


// start local simplification process

LocalSimplification::LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme, Polyhedron* poly)
{
	this->num_remove = 0;
	this->extreme_remove = Extremes2remove(extremes, reserved_extreme);
	//this->superlevelSetList.resize(num_remove);
	superlevel_set_propagation(poly);
	Degreecalc();
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
			if (q->verts[j]->scalar > max) {
				max = q->verts[j]->scalar;
				current_node = q->verts[j];
			}
		}

		extreme_scalar = current_node->scalar;
		while (true) {
			current_node->findNeighbor();

			//iteratively add neighbors of current node to neighbors list
			for (int j = 0; j < current_node->num_neighbors; j++) {
				if ((std::find(neighbors.begin(), neighbors.end(), current_node->neighbors[j]) == neighbors.end()) && //if it is not in neighbors list
					(std::find(visited.begin(), visited.end(), current_node->neighbors[j]) == visited.end())) //if not find, which means its unvisited
					neighbors.push_back(current_node->neighbors[j]);
			}
			// move to largest neighbor
			nextnode = LargestNeighbor(neighbors);

			if (nextnode == NULL)
				break;
			
			//if next node scalar value is larger than the maximum of extreme point, then stop propagation
			if (nextnode->scalar > extreme_scalar)
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
		if(visited.size() > 1)
			superlevelSetList.push_back(visited);
	}
}


/*
	this function should generate new ordering g as depicted in paper
	g should correspond to each superlevel set at each extreme point
*/
std::vector<Vertex*> LocalSimplification::LocalTopologicalSimplification()
{
	std::vector<Vertex*> g; // reordered g 
	//first initialize l_0 based on global order f
	std::vector<Vertex*> f;
	//Initializef();

	//go through each superlevel set and generate new ordering
	//after calculating new ordering g, assign scalars based on g
	for (int i = 0; i < this->superlevelSetList.size(); i++)
	{
		int length = superlevelSetList[i].size();
		std::vector<double> values = getValue(superlevelSetList[i]); // should be a list of scalars, leave this for now
		g = reorder(superlevelSetList[i]);
		g = assign(values, g); //assign values based on new ordering g
	}

	return g;
}


/*
assign values back to the new ordering g
*/
std::vector<Vertex*> LocalSimplification::assign(std::vector<double> values, std::vector<Vertex*> g) {
	std::sort(values.begin(), values.end(), std::greater<double>());
	for (int i = 0; i < g.size(); i++)
		g[i]->scalar = values[i];
	
	return g;
}


/*
get all scalars from one superlevel set,
values would be used later to assign to new ordering g
*/
std::vector<double> LocalSimplification::getValue(std::vector<Vertex*> f) {
	std::vector<double> values;
	for (int i = 0; i < f.size(); i++)
		values.push_back(f[i]->scalar);
	return values;
}

/*
actual reorder method
f: superlevel set
*/
std::vector<Vertex*> LocalSimplification::reorder(std::vector<Vertex*> f)
{
	std::vector<Vertex*> g, f_prime, f_primeNext;
	int maxIteration = 100;
	bool authorized = false;
	std::vector<Vertex*> authorized_min; // there could be multiple authorized_min

	//first set the value of saddle to be infinity and a authorized minimum as -infinity
	//initially saddle should be at the end of f
	f.back()->scalar = INFINITY;

	Vertex *authorized_max = f.back();

	//pick a authorized_min that would not seperate superlevel set 
	authorized_min = Authorized_minAssign(f, authorized_max);

	f_prime = f;
	int counter = 0;
	// forward pass and backward pass reordering as described in paper
	while (counter < maxIteration) {
		f_prime = this->positiveDirection(f_prime, authorized_max, authorized_min);
		authorized = this->checkExtremeLegal(f_prime, authorized_max, authorized_min);
		if (authorized == true) 
		{
			g = f_prime;
			break;
		}
		f_primeNext = this->reverseDirection(f_prime, authorized_max, authorized_min);
		authorized = this->checkExtremeLegal(f_primeNext, authorized_max, authorized_min);
		if (authorized == true)
		{
			g = f_primeNext;
			break;
		}
		else
			f_prime = f_primeNext;
		counter += 1;
	}

	return g;
}


/*
set previous saddle as the only maximum
go from the only maximum(saddle), continuously pick largest neighbor within superlevel set
update the order based on the sequence of selection
*/
std::vector<Vertex*> LocalSimplification::positiveDirection(std::vector<Vertex*> f_prime, Vertex* authorized_max, std::vector<Vertex*> &authorized_min)
{
	std::vector<Vertex*> f_primeNext;
	std::vector<Vertex*> neighbors;
	Vertex *current, *next;
	current = f_prime.back(); // start from saddle should be authorized_max
	int length = 0;
	double max = -INFINITY;
	int idx = 0;

	while(f_primeNext.size() < f_prime.size())
	{	
		max = -INFINITY;
		//first update neighbors list
		for (int i = 0; i < current->num_neighbors; i++) {
			if ((std::find(f_prime.begin(), f_prime.end(), current->neighbors[i]) != f_prime.end()) && // if neighbor of current node is in superlevel set
				(std::find(neighbors.begin(), neighbors.end(), current->neighbors[i]) == neighbors.end()) && // if it is not in current neighbor list
				(std::find(f_primeNext.begin(), f_primeNext.end(), current->neighbors[i]) == f_primeNext.end()) // if it is not visited
				)
				neighbors.push_back(current->neighbors[i]);
		}

		//select the largest neighbor in neighbors
		for (int i = 0; i < neighbors.size(); i++) 
		{
			if ((neighbors[i]->scalar >= max))
			{
				max = neighbors[i]->scalar;
				next = neighbors[i];
			}
		}

		f_primeNext.push_back(current);

		current = next;
		neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), next), neighbors.end());
		next = NULL;

	}

	//assign temporary value i based on its order to vertex->scalar as described in paper
	for (int i = 0; i < f_primeNext.size(); i++)
	{
		if (std::find(authorized_min.begin(), authorized_min.end(), f_primeNext[i]) == authorized_min.end() &&
			(f_primeNext[i] != authorized_max)
			) // if the node is not authorized max or authorized min
			f_primeNext[i]->scalar = f_primeNext.size() - i;
	}

	return f_primeNext;
}


/*
set authorized minimum as starting point
go from the authorized minimum, continuously pick smallest neighbor within superlevel set
update the order based on the sequence of selection
*/
std::vector<Vertex*> LocalSimplification::reverseDirection(std::vector<Vertex*> f_prime, Vertex* authorized_max, std::vector<Vertex*> &authorized_min)
{
	std::vector<Vertex*> f_primeNext;
	Vertex* current, * next;
	std::vector<Vertex*> neighbors;
	current = f_prime.back(); //start from authorized min, should be at the end of the list
	int length = 0;
	double min = INFINITY;
	int idx = 0;

	/*
	//in sublevel propagate, there maybe multiple authorized min, so we need all neighbors of all authorized min at the beginning
	for (int i = 0; i < authorized_min.size(); i++)
	{
		for (int j = 0; j < authorized_min[i]->num_neighbors; j++)
		{
			if (std::find(f_prime.begin(), f_prime.end(), authorized_min[i]->neighbors[j]) != f_prime.end() && // if neighbor of current node is in superlevel set
				(std::find(neighbors.begin(), neighbors.end(), authorized_min[i]->neighbors[j]) == neighbors.end()) // if it is not in current neighbor list
				)
				neighbors.push_back(authorized_min[i]->neighbors[j]);
		}
		
		if (i!=0) // I add this because first node is current and it will be push into f_primeNext in the while loop
			f_primeNext.push_back(authorized_min[i]);
	}
	*/

	while (f_primeNext.size() < f_prime.size())
	{
		min = INFINITY;
		//first update neighbors list
		for (int i = 0; i < current->num_neighbors; i++) {
			if ((std::find(f_prime.begin(), f_prime.end(), current->neighbors[i]) != f_prime.end()) && // if neighbor of current node is in superlevel set
				(std::find(neighbors.begin(), neighbors.end(), current->neighbors[i]) == neighbors.end()) && // if it is not in current neighbor list
				(std::find(f_primeNext.begin(), f_primeNext.end(), current->neighbors[i]) == f_primeNext.end()) // if it is not visited
				)
				neighbors.push_back(current->neighbors[i]);
		}

		//select the smallest neighbor in superlevel set
		for (int i = 0; i < neighbors.size(); i++) {
			if (neighbors[i]->scalar <= min) 
			{
				min = neighbors[i]->scalar;
				next = neighbors[i];
			}
		}

		f_primeNext.push_back(current);
		current = next;

		neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), next), neighbors.end());
		next = NULL;
	}

	for (int i = 0; i < f_primeNext.size(); i++)
	{
		if (std::find(authorized_min.begin(), authorized_min.end(), f_primeNext[i]) == authorized_min.end() &&
			(f_primeNext[i] != authorized_max)
			) // if the node is not authorized max or authorized min
			f_primeNext[i]->scalar = i;
	}

	return f_primeNext;
}

/*
check if unauthorized extreme points exist or not
*/
bool LocalSimplification::checkExtremeLegal(std::vector<Vertex*> order, Vertex *authorized_max, std::vector<Vertex*> authorized_min)
{
	bool test = true;
	Vertex* current;
	for (int i = 0; i < order.size(); i++)
	{
		current = order[i];
		std::vector<Vertex*> neighborInside;
		//first get all neighbors inside superlevel set
		for (int j = 0; j < current->num_neighbors; j++) 
		{
			if (std::find(order.begin(), order.end(), current->neighbors[j]) != order.end())
				neighborInside.push_back(current->neighbors[j]);
		}

		if (current != authorized_max && (std::find(authorized_min.begin(),authorized_min.end(), current) == authorized_min.end()))//if current is not authorized max or min
		{
			bool isMax = true, isMin = true;
			//check if it is local maxima
			for (int j = 0; j < neighborInside.size(); j++)
			{
				if (neighborInside[j]->scalar > current->scalar)
					isMax = false;
				if (neighborInside[j]->scalar < current->scalar)
					isMin = false;
			}

			if (isMax) //local maxima not allowed
			{
				test = false;
				continue;
			}
			if (isMin)//if it is local minima, it has to live on the boundary of superlevel set
			{
				if (current->degreeInSuperLevelset == 4)// if the node is inside the set
					test = false;

			}
		}
	}

	return test;
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

	if (neighbors.size() == 0)
		return NULL;

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


/*
Initialize global ordering f for each superlevel set
basically sort all vertex* base on its scalar value

*/
/*
void LocalSimplification::Initializef() 
{
	for (int i = 0; i < this->superlevelSetList.size(); i++)
	{
		std::vector<Vertex*> superlevelset = this->superlevelSetList[i];
		std::sort(superlevelset.begin(), superlevelset.end(), compare); //
		this->superlevelSetList[i] = superlevelset;
	}

}
*/

/*
helper function for sort in Initializef()
*/
bool compare(const Vertex* a, const Vertex* b)
{
	return a->scalar < b->scalar;
}
	

/*
helper function for calculating degress of nodes in each superlevel set
*/

void LocalSimplification::Degreecalc()
{
	for (int i = 0; i < superlevelSetList.size(); i++)
	{
		for (int j = 0; j < superlevelSetList[i].size(); j++)
		{
			Vertex* current = superlevelSetList[i][j];
			for (int k = 0; k < current->num_neighbors; k++)
			{
				if (std::find(superlevelSetList[i].begin(), superlevelSetList[i].end(), current->neighbors[k]) != superlevelSetList[i].end()) // if find in superlevel set
					current->degreeInSuperLevelset += 1;
			}
			
		}
	}
}

std::vector<Vertex*> Authorized_minAssign(std::vector<Vertex*> f, Vertex* authorized_max)
{
	bool find = false;
	std::vector<Vertex*> authorized_min;
	if (f.size() == 2)
	{
		f[0]->scalar = -INFINITY;
		authorized_min.push_back(f[0]);
		return authorized_min;
	}
	
	
	for (int i = 0; i < f.size(); i++)
	{
		if ((f[i]->degreeInSuperLevelset == 1) &&
			(f[i] != authorized_max)
			)
		{
			f[i]->scalar = -INFINITY;
			authorized_min.push_back(f[i]);
			return authorized_min;
		}

	}

	int i = 0;
	while (!find && i<f.size())
	{
		for (int j = 0; j < f[i]->num_neighbors; j++)
		{
			if ((std::find(f.begin(), f.end(), f[i]->neighbors[j]) == f.end()) &&
				(f[i] != authorized_max) &&
				(f[i]->neighbors[j]->degreeInSuperLevelset >= 3)
				)// if a node has a neighbor outside f and it is not authorized_max, plus it has neighbor with more than 3 neighbors (so picking it won't separate the set)
			{
				f[i]->scalar = -INFINITY;
				authorized_min.push_back(f[i]);
				find = true;
				break;
			}
		}
		i += 1;
	}
	
	return authorized_min;
}
