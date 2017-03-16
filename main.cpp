#include <iostream>
#include <fstream>
#include "Cell.h"
#include "Generation.h"

#define ARG_SIZE 3

int main(int argc, char* argv[]) {

    // Verify initial argument correct size
    if (argc != ARG_SIZE) {
        std::cout << "Usage: life3d <filename> <nr of generations>" << std::endl;
        return -1;
    }

    // Get filename
    std::string fileName = argv[1];

    // Get number of iterations
    int nrIterations = std::stoi(argv[2]);

    std::unordered_set<Cell> initialGeneration;

    // Read file
    std::ifstream infile(fileName + ".in");
    int x, y, z, sideSize;
    infile >> sideSize;

    while (infile >> x >> y >> z) {
        Cell cell(x, y, z);
        initialGeneration.insert(cell);
    }

    Generation generation(nrIterations, sideSize, initialGeneration);
    generation.evolve();

    std::cout << generation << std::endl;

    return 0;
}