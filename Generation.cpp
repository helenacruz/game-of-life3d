//
// Created by goncalo on 16-03-2017.
//

#include <iostream>
#include "Generation.h"


Generation::Generation(int nrGenerations, int sideSize, std::unordered_set<Cell> initialGenerationCells) {
    this->nrGenerations = nrGenerations;
    this->sideSize = sideSize;
    this->currentGeneration = initialGenerationCells;
}

void Generation::evolve() {

}

std::ostream& operator<<(std::ostream& os, const Generation& obj) {
    // sort current generation by x,y,z index

    for(auto it = currentGeneration.begin(); it != currentGeneration.end(); ++it){
        std::cout << *it << std::endl;
    }
    // write obj to stream
    return os;
}
