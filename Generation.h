//
// Created by goncalo on 16-03-2017.
//

#ifndef LIFE3D_GENERATION_H
#define LIFE3D_GENERATION_H


#include <map>
#include <unordered_set>
#include "Cell.h"

class Generation {
private:
    int nrGenerations;
    int sideSize;

    std::map<Cell, int> currentDeadCells;
    std::unordered_set<Cell, Cell::hash> currentGeneration;
    std::unordered_set<Cell, Cell::hash> nextGeneration;

public:
    Generation(int nrGenerations, int sideSize, std::unordered_set<Cell, Cell::hash> initialGenerationCells);
    void evolve();
    friend std::ostream& operator<<(std::ostream& os, const Generation& obj);
};


#endif //LIFE3D_GENERATION_H
