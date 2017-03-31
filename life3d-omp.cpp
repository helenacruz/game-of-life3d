#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <omp.h>
#include <cmath>

#define ARG_SIZE 3
#define CHUNK_SIZE 2

class Cell
{
private:

    int x;
    int y;
    int z;

public:

    Cell() {
    
    }

    Cell(int x, int y, int z) : x(x), y(y), z(z) {

    }

    inline int getX() const {
        return x;
    }

    inline int getY() const {
        return y;
    }

    inline int getZ() const {
        return z;
    }

    inline bool operator!=(Cell const &cell) const { 
        return !(*this == cell); 
    }

    inline bool operator==(const Cell &cell) const {
        return x == cell.x &&
               y == cell.y &&
               z == cell.z;
    }

    inline bool operator<(const Cell &cell) const {
        return std::tie(x, y, z) < std::tie(cell.x, cell.y, cell.z);
    }

    inline friend std::ostream &operator<<(std::ostream &os, const Cell &cell) {
        os <<  cell.x << " " << cell.y << " " << cell.z;
        return os;
    }

    struct hash {
        size_t operator()(Cell const &x) const noexcept
        {
            return ((51 + std::hash<int>()(x.getX())) *
                    (51 + std::hash<int>()(x.getY())) *
                    (51 + std::hash<int>()(x.getZ()))
            );
        }
    };
};

int size, nrThreads;

// new data structures
typedef std::unordered_set<Cell, Cell::hash> CellSet;
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;
std::vector<CellSet> currentGeneration;
std::vector<CellSet> nextGeneration;
std::vector<DeadMap> deadCells;


int getIndex(Cell cell);
void evolve();
int getNeighbors(Cell cell, int i);

inline void printResults();
inline void printCells(std::vector<Cell> &cells);
inline void printCells(std::unordered_set<Cell, Cell::hash> &cells);
inline void printCells(std::unordered_map<Cell, int, Cell::hash> &cells);

int main(int argc, char* argv[]) {

    if (argc != ARG_SIZE) {
        std::cout << "Usage: life3d <filename> <nr of generations>" << std::endl;
        return -1;
    }

    double start = omp_get_wtime();
    // Input reading
    std::string filename = argv[1];
    int nrGenerations = std::stoi(argv[2]);


    // Distribute cells to according index in currentGeneration
    std::ifstream infile(filename);
    infile >> size;
    int x, y, z;

    // data structure initialization
    nrThreads = omp_get_num_procs();

    for(int i = 0; i < nrThreads * CHUNK_SIZE; i++){
        CellSet set;
        currentGeneration.push_back(set);
    }
    //std::cout << "size " << nrThreads << std::endl;
    double nrInChargeX = (double) size / (double) (nrThreads * CHUNK_SIZE);
    double nrInChargeY = (double) size / (double) CHUNK_SIZE;
    while (infile >> x >> y >> z) {
        // Cell definition and insertion at data structures
        Cell cell(x, y, z);
        int index = 0;
        if(size < nrThreads * CHUNK_SIZE){
            int indexX = (int) (x / nrInChargeX);
            int indexY = (int) (y / nrInChargeY);
            index = indexX + indexY;
        }else{
            int indexX = (x / (size / CHUNK_SIZE))* CHUNK_SIZE;
            int indexY = (int) (y / nrInChargeY);
            index = indexX + indexY;
        }
        // We have to insert the cell at an already existent set item
        currentGeneration.at((unsigned long) index).insert(cell);
        //std::cout << "ELAPSED TIME: " << currentGeneration << std::endl;
    }

    std::cout << "nrGenerations" << nrGenerations << std::endl;
    for (int i = 0; i < nrGenerations; i++) {;
        std::cout << "generation " << i << std::endl;
        evolve();
    }

    double end = omp_get_wtime();
    //printResults();

    std::cout << "ELAPSED TIME: " << (end - start) << std::endl;
    fflush(stdout);
    return 0;
}

int getIndex(Cell cell){
    int x = cell.getX();
    int y = cell.getY();
    int z = cell.getZ();

    double nrInChargeX = (double) size / (double) (nrThreads * CHUNK_SIZE);
    double nrInChargeY = (double) size / (double) CHUNK_SIZE;
    int index = 0;
    if(size < nrThreads * CHUNK_SIZE){
        int indexX = (int) (x / nrInChargeX);
        int indexY = (int) (y / nrInChargeY);
        index = indexX + indexY;
    }else{
        int indexX = (x / (size / CHUNK_SIZE))* CHUNK_SIZE;
        int indexY = (int) (y / nrInChargeY);
        index = indexX + indexY;
    }
    return index;
}

void evolve() {
    //std::cout << "evolve: " << currentGeneration.size() << std::endl;
    // Allocate space for next generation
    for(int i = 0; i < nrThreads * CHUNK_SIZE; i++){
        CellSet set;
        nextGeneration.push_back(set);

        DeadMap deadMap;
        deadCells.push_back(deadMap);
    }


    #pragma omp parallel
    {
        // We will divide the current generation vector sets dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK_SIZE)
        for (unsigned int i = 0; i < currentGeneration.size(); i++) {
            // Each thread iterates through a set...
            CellSet &set = currentGeneration.at(i);
            for(auto it = set.begin(); it != set.end(); ++it){

                // Get nr of neighbours and decide if stays alive or not

                int neighbors;
                #pragma omp critical (first)
                {
                    neighbors = getNeighbors(*it, i);
                }


                if (neighbors >= 2 && neighbors <= 4) {
                    // with 2 to 4 neighbors the cell lives
                    #pragma omp critical (second)
                    {
                        nextGeneration.at((unsigned long) getIndex(*it)).insert(*it);
                    };

                }
            }
        }

        // We will also divide the dead cells map dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK_SIZE)
        for (unsigned int i = 0; i < deadCells.size(); i++) {
            // Each thread iterates through a map
            DeadMap &map = deadCells.at(i);

            // Iterate through dead cells counters and if validate condition add cell as an alive one
            // to the next generation.
            // TODO: verify at which index of vector nextgeneration to add the cell accordingly to the rule previously used
            for(auto it = map.begin(); it!= map.end(); ++it){
                if (it->second == 2 || it->second == 3) {
                    #pragma omp critical (third)
                    {
                        nextGeneration.at((unsigned long) getIndex(it->first)).insert(it->first);
                    };

                }
            }
        }
    }
    std::cout << "evolve" << std::endl;
    fflush(stdout);
    currentGeneration = nextGeneration; // new generation is our current generation
    nextGeneration.clear(); // clears new generation
    deadCells.clear(); // clears dead cells from previous generation

}

int getNeighbors(Cell cell, int vectorIndex) {
    unsigned long cell1index, cell2index, cell3index, cell4index, cell5index, cell6index;
    int nrNeighbors = 0;
    int x = cell.getX();
    int y = cell.getY();
    int z = cell.getZ();
    int xx, yy, zz;

    Cell *cell1 = new Cell();
    Cell *cell2 = new Cell();
    Cell *cell3 = new Cell();
    Cell *cell4 = new Cell();
    Cell *cell5 = new Cell();
    Cell *cell6 = new Cell();

    // X Index limits correction
    if (x - 1 < 0) {
        xx = size - 1;
        *cell1 = Cell(xx, y, z);
    }
    else {
        *cell1 = Cell(x - 1, y, z);
    }

    if (x + 1 >= size) {
        xx = 0;
        *cell2 = Cell(xx, y, z);
    }
    else {
        *cell2 = Cell(x + 1, y, z);
    }

    // y Index limits correction
    if (y - 1 < 0) {
        yy = size - 1;
        *cell3 = Cell(x, yy, z);
    }
    else {
        *cell3 = Cell(x, y - 1, z);
    }

    if (y + 1 >= size) {
        yy = 0;
        *cell4 = Cell(x, yy, z);
    }
    else {
        *cell4 = Cell(x, y + 1, z);
    }

    // z Index limits correction
    if (z - 1 < 0) {
        zz = size - 1;
        *cell5 = Cell(x, y, zz);
    }
    else {
        *cell5 = Cell(x, y, z - 1);
    }

    if (z + 1 >= size) {
        zz = 0;
        *cell6 = Cell(x, y, zz);
    }
    else {
        *cell6 = Cell(x, y, z + 1);
    }

    /*
     * UNCOMMENT THIS FOR INDEX VERIFICATION
    std::cout << "index-> " << vectorIndex << ": cell1: " << getIndex(cell1.getX(), cell1.getY(), cell1.getZ())
            << ": cell1: " << getIndex(cell2.getX(), cell2.getY(), cell2.getZ())
            << ": cell1: " << getIndex(cell3.getX(), cell3.getY(), cell3.getZ())
            << ": cell1: " << getIndex(cell4.getX(), cell4.getY(), cell4.getZ())
            << ": cell1: " << getIndex(cell5.getX(), cell5.getY(), cell5.getZ())
            << ": cell1: " << getIndex(cell6.getX(), cell6.getY(), cell6.getZ()) << std::endl;

    fflush(stdout);
     */
    cell1index = (unsigned long) getIndex(*cell1);
    cell2index = (unsigned long) getIndex(*cell2);
    cell3index = (unsigned long) getIndex(*cell3);
    cell4index = (unsigned long) getIndex(*cell4);
    cell5index = (unsigned long) getIndex(*cell5);
    cell6index = (unsigned long) getIndex(*cell6);

    if (currentGeneration.at(cell1index).count(*cell1) > 0) {
        nrNeighbors++;
        // std::cout << "cell1" << std::endl;
    }
    else {
        // TODO: Missing space allocation if cell is not present in deadCells.at(cellIndex)!!!
        deadCells.at(cell1index)[*cell1] += 1;
        // std::cout << "dead cell1 neighbor: " << cell1 << std::endl;
    }
    if (currentGeneration.at(cell2index).count(*cell2) > 0) {
        nrNeighbors++;
        // std::cout << "cell2" << std::endl;
    }
    else {
        deadCells.at(cell2index)[*cell2] += 1;
        // std::cout << "dead cell2 neighbor: " << cell2 << std::endl;
    }
    if (currentGeneration.at(cell3index).count(*cell3) > 0) {
        nrNeighbors++;
        // std::cout << "cell3" << std::endl;
    }
    else {
        //nr = deadCells.at(cell3index)[cell1];
        deadCells.at(cell3index)[*cell1] += 1;
        // std::cout << "dead cell3 neighbor: " << cell3 << std::endl;
    }
    if (currentGeneration.at(cell4index).count(*cell4) > 0) {
        nrNeighbors++;
        // std::cout << "cell4" << std::endl;
    }
    else {
        deadCells.at(cell4index)[*cell1] += 1;
        // std::cout << "dead cell4 neighbor: " << cell4 << std::endl;
    }
    if (currentGeneration.at(cell5index).count(*cell5) > 0) {
        nrNeighbors++;
        // std::cout << "cell5" << std::endl;
    }
    else {
        deadCells.at(cell5index)[*cell1] += 1;
        // std::cout << "dead cell5 neighbor: " << cell5 << std::endl;
    }
    if (currentGeneration.at(cell6index).count(*cell6) > 0) {
        nrNeighbors++;
        // std::cout << "cell6" << std::endl;
    }
    else {
        deadCells.at(cell6index)[*cell1] += 1;
        // std::cout << "dead cell6 neighbor: " << cell6 << std::endl;
    }


    delete(cell1);
    delete(cell2);
    delete(cell3);
    delete(cell4);
    delete(cell5);
    delete(cell6);
    return nrNeighbors;
}

/* Aux functions for printing data */

inline void printResults() {

    unsigned int genSize = 0;
    for(auto genIt = currentGeneration.begin(); genIt != currentGeneration.end(); ++genIt){
        CellSet set = *genIt;
        genSize += set.size();
        for(auto setIt = set.begin(); setIt != set.end(); ++setIt){
            std::cout << *setIt << std::endl;
        }
    }

    std::cout << "size: " << genSize << std::endl;
}

inline void printCells(std::vector<Cell> &cells)
{
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        std::cout << *it;
    }
}

inline void printCells(std::unordered_set<Cell, Cell::hash> &cells)
{
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        std::cout << *it;
    }
}

inline void printCells(std::unordered_map<Cell, int, Cell::hash> &cells) {
    /*
    for (auto it = deadCells.begin(); it != deadCells.end(); ++it) {
        //std::cout << "cell: " << it->first << std::endl;
        //std::cout << "nr: " << it->second << std::endl;
    }*/
}
/*
 * Usar o schedule dynamic [,chunk]
 * Temos de encontrar o numero optimo de chunks: analizar o size e o nº de threads para distribuir os calculos
 * Nº sets = nº chunks * nº threads
 * Inicializar os sets
 * Criar um vector de unordered sets para definir quais os sets que cada thread deve tratar (buscar os sets de acordo com o indice do vector-pensar no indices)
 * 
 * Na ultima generation, juntar todos os unordered_sets criados previamente num set ordenado (para devolver o resultado ordenado)
 * 
 * Escrever codigo
 * Teclar codigo
 * Codar
 * Pensar
 */
