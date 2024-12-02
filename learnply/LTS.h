#pragma once
#include "project2.h"
#include <algorithm>

//local topological simplification
// extreme_remove: extremes to remove
//superlevelSetList: list of superlevel set, lenth is the number of extremes to remove, each superlevel set should be vertex list f as mentioned in paper 
//num_remove: number of local extreme points to remove

class LocalSimplification {
private:
	int num_remove;
	std::vector<CriticalPoint> extreme_remove;
	//std::vector<std::vector<Vertex*>> neighborlist; 
	std::vector<std::vector<Vertex*>> superlevelSetList; // this is nasty, may reconstruct later

public:
	LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme, Polyhedron* poly);
	~LocalSimplification();

	// implement a superlevel set propagation as mentioned in paper, can be invoked in constructor
	void superlevel_set_propagation(Polyhedron* poly);

	//reordering method as mentioned in paper, should return reordering g
	std::vector<Vertex> LocalTopologicalSimplification();

	//merge g with original f
	std::vector<Vertex> merge(Vertex** f, std::vector<Vertex> g);

	Vertex* LargestNeighbor(std::vector<Vertex*>);

private:
	//get all extremes to remove, should be used in constructor
	std::vector<CriticalPoint> Extremes2remove(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme);

};


