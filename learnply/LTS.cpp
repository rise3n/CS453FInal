#include "LTS.h"



// start local simplification process
void local_simplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme)
{

}

LocalSimplification::LocalSimplification(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme)
{
	this->num_remove = extremes.size() - reserved_extreme.size();

}

LocalSimplification::~LocalSimplification()
{

}

void LocalSimplification::superlevel_set_propagation(Polyhedron* poly) 
{





}


std::vector<Vertex> LocalSimplification::LocalTopologicalSimplification()
{





}

std::vector<Vertex> LocalSimplification::Extremes2remove(std::vector<CriticalPoint> extremes, std::vector<CriticalPoint> reserved_extreme)
{




}

std::vector<Vertex> LocalSimplification::merge(Vertex** f, std::vector<Vertex> g)
{





}
