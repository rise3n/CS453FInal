#pragma once
#include "project2.h"
#include <algorithm>
#include <random>
#include <mutex>

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
	LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme, Polyhedron* poly, bool);
	~LocalSimplification();

	// implement a superlevel set propagation as mentioned in paper, can be invoked in constructor
	void parallel_superlevel_set_propagation(Polyhedron* poly);
	void superlevel_set_propagation(Polyhedron* poly);

	//reordering method as mentioned in paper, should return reordering g
	std::vector<Vertex*> LocalTopologicalSimplification();

	//merge g with original f
	std::vector<Vertex> merge(Vertex** f, std::vector<Vertex> g);

	Vertex* LargestNeighbor(std::vector<Vertex*>);

private:
	//get all extremes to remove, should be used in constructor
	std::vector<CriticalPoint> Extremes2remove(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme);
	//void Initializef();
	std::vector<Vertex*> reorder(std::vector<Vertex*>);
	std::vector<Vertex*> positiveDirection(std::vector<Vertex*>, Vertex*, std::vector<Vertex*>&);
	std::vector<Vertex*> reverseDirection(std::vector<Vertex*>, Vertex*, std::vector<Vertex*>&);
	std::vector<Vertex*> assign(std::vector<double>, std::vector<Vertex*>, int);
	std::vector<double> getValue(std::vector<Vertex*>);
	void Degreecalc();
	void singleSuperLevelProp(Polyhedron*, CriticalPoint, int, std::mutex&);

	// check if reordered sequence has unauthorized extremes, if exist, then return yes, else no
	bool checkExtremeLegal(std::vector<Vertex*>, Vertex*, std::vector<Vertex*>);
	void eliminateDupSet();
};

bool compare(const Vertex*, const Vertex*);
std::vector<Vertex*> Authorized_minAssign(std::vector<Vertex*>, Vertex*);



