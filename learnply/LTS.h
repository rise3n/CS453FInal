#pragma once
#include "project2.h"

//local topological simplification
// extreme_remove: extremes to remove
//superlevelSetList: list of superlevel set, lenth is the number of extremes to remove, each superlevel set should be vertex list f as mentioned in paper 
//num_remove: number of local extreme points to remove

class LocalSimplification {
private:
	int num_remove;
	std::vector<Vertex> extreme_remove;
	std::vector<Vertex> superlevelSetList;

public:
	LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme);
	~LocalSimplification();

	// implement a superlevel set propagation as mentioned in paper, can be invoked in constructor
	void superlevel_set_propagation(Polyhedron* poly);

	//reordering method as mentioned in paper, should return reordering g
	std::vector<Vertex> LocalTopologicalSimplification();

	//merge g with original f
	std::vector<Vertex> merge(Vertex** f, std::vector<Vertex> g);

private:
	//get all extremes to remove, should be used in constructor
	std::vector<Vertex> Extremes2remove(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme);

};

