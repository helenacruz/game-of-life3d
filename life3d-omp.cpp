#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <omp.h>

#define ARG_SIZE 3
#define NR_SETS 32
#define CHUNK 8

inline int generateIndex(int x, int y, int z);

class Cell
{
private:

    int x;
    int y;
    int z;
    int index;

public:

    Cell() {
    
    }

    Cell(int x, int y, int z) : x(x), y(y), z(z) {
        index = generateIndex(x, y, z);
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

    inline int getIndex() const {
        return index;
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
                    (51 + 2*std::hash<int>()(x.getY())) *
                    (51 + 4*std::hash<int>()(x.getZ()))
            );
        }
    };
};

int size;
int index = 0;

// new data structures
typedef std::unordered_set<Cell, Cell::hash> CellSet;
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;

std::vector<CellSet> currentGeneration(NR_SETS);
std::vector<CellSet> nextGeneration(NR_SETS);
std::vector<DeadMap> deadCells(NR_SETS);

void evolve();
int getNeighbors(Cell cell, int i);
void insertNextGeneration(Cell cell);
void insertDeadCell(Cell cell);

inline void initializeVector(std::vector<CellSet> &sets);
inline void initializeMap(std::vector<DeadMap> &maps);
inline void printResults();

int main(int argc, char* argv[]) {

    if (argc != ARG_SIZE) {
        std::cout << "Usage: life3d <filename> <nr of generations>" << std::endl;
        return -1;
    }

    double start = omp_get_wtime();
    
    std::string filename = argv[1];
    int nrGenerations = std::stoi(argv[2]);

    std::ifstream infile(filename);
    infile >> size;
    int x, y, z;


    while (infile >> x >> y >> z) {
        Cell cell(x, y, z);
        currentGeneration[cell.getIndex()].insert(cell);
    }


    for (int i = 0; i < nrGenerations; i++) {
        evolve();
    }
    
    printResults();

    double end = omp_get_wtime();
    //std::cout << "Time: " << (end - start) << std::endl;

    return 0;
}

inline void initializeMap(std::vector<DeadMap> &maps) {
    for (int i = 0; i < NR_SETS; i++) {
        DeadMap map;
        maps[i] = map;
    }
}

inline int generateIndex(int x, int y, int z) {
    /*
    return ((51 + std::hash<int>()(x)) *
            (51 + std::hash<int>()(y)) *
            (51 + std::hash<int>()(z))) % NR_SETS;*/

    double nrInChargeX = (double) size / (double) (NR_SETS);
    double nrInChargeY = (double) size / (double) CHUNK;
    int index = 0, indexX = 0;
    int indexY = (int) (y / nrInChargeY);
    if(size < NR_SETS){
        indexX = (int) (x / nrInChargeX);
        index = (indexX + indexY) %((NR_SETS));
    }else{
        index = (x * NR_SETS) / size;
    }


    return index;
}

void evolve() {
    #pragma omp parallel
    {
        // We will divide the current generation vector sets dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK)
        for (int i = 0; i < NR_SETS; i++) {
            // Each thread iterates through a set...
            CellSet &set = currentGeneration[i];

            for (auto it = set.begin(); it != set.end(); ++it){
                int neighbors = getNeighbors(*it, i);
                if (neighbors >= 2 && neighbors <= 4) {
                    // with 2 to 4 neighbors the cell lives
                    insertNextGeneration(*it);
                }
            }
        }

        // We will also divide the dead cells map dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK)
        for (int i = 0; i < NR_SETS; i++) {
            // Each thread iterates through a map
            DeadMap &map = deadCells[i];

            for (auto it = map.begin(); it != map.end(); ++it){
                if (it->second == 2 || it->second == 3) {
                    insertNextGeneration(it->first);
                }
            }
        }
    }

    currentGeneration = std::move(nextGeneration); // new generation is our current generation
    nextGeneration = std::vector<CellSet>(NR_SETS);
    for (int i = 0; i < NR_SETS; i++) {
        initializeMap(deadCells); 
    }
}

int getNeighbors(Cell cell, int vectorIndex) {
    int nrNeighbors = 0;
    int x = cell.getX();
    int y = cell.getY();
    int z = cell.getZ();
    int xx, yy, zz;

    Cell cell1;
    Cell cell2;
    Cell cell3;
    Cell cell4;
    Cell cell5;
    Cell cell6;

    if (x - 1 < 0) {
        xx = size - 1;
        cell1 = Cell(xx, y, z);
    }
    else {
        cell1 = Cell(x - 1, y, z);
    }

    if (x + 1 >= size) {
        xx = 0;
        cell2 = Cell(xx, y, z);
    }
    else {
        cell2 = Cell(x + 1, y, z);
    }

    if (y - 1 < 0) {
        yy = size - 1;
        cell3 = Cell(x, yy, z);
    }
    else {
        cell3 = Cell(x, y - 1, z);
    }

    if (y + 1 >= size) {
        yy = 0;
        cell4 = Cell(x, yy, z);
    }
    else {
        cell4 = Cell(x, y + 1, z);
    }

    if (z - 1 < 0) {
        zz = size - 1;
        cell5 = Cell(x, y, zz);
    }
    else {
        cell5 = Cell(x, y, z - 1);
    }

    if (z + 1 >= size) {
        zz = 0;
        cell6 = Cell(x, y, zz);
    }
    else {
        cell6 = Cell(x, y, z + 1);
    }
    
    if (currentGeneration[cell1.getIndex()].count(cell1) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell1);
    }
    
    if (currentGeneration[cell2.getIndex()].count(cell2) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell2);
    }

    if (currentGeneration[cell3.getIndex()].count(cell3) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell3);
    }

    if (currentGeneration[cell4.getIndex()].count(cell4) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell4);
    }

    if (currentGeneration[cell5.getIndex()].count(cell5) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell5);
    }

    if (currentGeneration[cell6.getIndex()].count(cell6) > 0) {
        nrNeighbors++;
    }
    else {
        insertDeadCell(cell6);
    }

    return nrNeighbors;
}

void insertNextGeneration(Cell cell) {
    int index = cell.getIndex();

    switch(index){
        case 0 :
            #pragma omp critical (nextGeneration_0)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 1 :
            #pragma omp critical (nextGeneration_1)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 2 :
            #pragma omp critical (nextGeneration_2)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 3 :
            #pragma omp critical (nextGeneration_3)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 4 :
            #pragma omp critical (nextGeneration_4)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 5 :
            #pragma omp critical (nextGeneration_5)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 6 :
            #pragma omp critical (nextGeneration_6)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 7 :
            #pragma omp critical (nextGeneration_7)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 8 :
            #pragma omp critical (nextGeneration_8)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 9 :
            #pragma omp critical (nextGeneration_9)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 10 :
            #pragma omp critical (nextGeneration_10)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 11 :
            #pragma omp critical (nextGeneration_11)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 12 :
            #pragma omp critical (nextGeneration_12)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 13 :
            #pragma omp critical (nextGeneration_13)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 14 :
            #pragma omp critical (nextGeneration_14)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 15 :
            #pragma omp critical (nextGeneration_15)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 16 :
            #pragma omp critical (nextGeneration_16)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 17 :
            #pragma omp critical (nextGeneration_17)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 18 :
            #pragma omp critical (nextGeneration_18)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 19 :
            #pragma omp critical (nextGeneration_19)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 20 :
            #pragma omp critical (nextGeneration_20)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 21 :
            #pragma omp critical (nextGeneration_21)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 22 :
            #pragma omp critical (nextGeneration_22)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 23 :
            #pragma omp critical (nextGeneration_23)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 24 :
            #pragma omp critical (nextGeneration_24)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 25 :
            #pragma omp critical (nextGeneration_25)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 26 :
            #pragma omp critical (nextGeneration_26)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 27 :
            #pragma omp critical (nextGeneration_27)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 28 :
            #pragma omp critical (nextGeneration_28)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 29 :
            #pragma omp critical (nextGeneration_29)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 30 :
            #pragma omp critical (nextGeneration_30)
            {
                nextGeneration[index].insert(cell);
            }
            break;
        case 31 :
        #pragma omp critical (nextGeneration_31)
            {
                nextGeneration[index].insert(cell);
            }
            break;
    }
}

void insertDeadCell(Cell cell) {
    int index = cell.getIndex();

    switch(index){
        case 0 :
            #pragma omp critical (deadCells_0)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 1 :
            #pragma omp critical (deadCells_1)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 2 :
            #pragma omp critical (deadCells_2)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 3 :
            #pragma omp critical (deadCells_3)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 4 :
            #pragma omp critical (deadCells_4)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 5 :
            #pragma omp critical (deadCells_5)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 6 :
            #pragma omp critical (deadCells_6)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 7 :
            #pragma omp critical (deadCells_7)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 8 :
            #pragma omp critical (deadCells_8)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 9 :
            #pragma omp critical (deadCells_9)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 10 :
            #pragma omp critical (deadCells_10)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 11 :
            #pragma omp critical (deadCells_11)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 12 :
            #pragma omp critical (deadCells_12)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 13 :
            #pragma omp critical (deadCells_13)
            {
                deadCells[index][cell] += 1;
            }
                break;
        case 14 :
            #pragma omp critical (deadCells_14)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 15 :
            #pragma omp critical (deadCells_15)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 16 :
            #pragma omp critical (deadCells_16)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 17 :
            #pragma omp critical (deadCells_17)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 18 :
            #pragma omp critical (deadCells_18)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 19 :
            #pragma omp critical (deadCells_19)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 20 :
            #pragma omp critical (deadCells_20)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 21 :
            #pragma omp critical (deadCells_21)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 22 :
            #pragma omp critical (deadCells_22)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 23 :
            #pragma omp critical (deadCells_23)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 24 :
            #pragma omp critical (deadCells_24)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 25 :
        #pragma omp critical (deadCells_25)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 26 :
            #pragma omp critical (deadCells_26)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 27 :
            #pragma omp critical (deadCells_27)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 28 :
            #pragma omp critical (deadCells_28)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 29 :
            #pragma omp critical (deadCells_29)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 30 :
            #pragma omp critical (deadCells_30)
            {
                deadCells[index][cell] += 1;
            }
            break;
        case 31 :
            #pragma omp critical (deadCells_31)
            {
                deadCells[index][cell] += 1;
            }
            break;
    }
}

/* Aux functions for printing data */

inline void printResults() {
    std::set<Cell> lastGeneration;

    for (int i = 0; i < NR_SETS; i++) { 
        lastGeneration.insert(currentGeneration[i].begin(),
                              currentGeneration[i].end());
    }

    for (auto it = lastGeneration.begin(); it != lastGeneration.end(); ++it) {
        std::cout << *it << std::endl;
    }
}

