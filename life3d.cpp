#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

#define ARG_SIZE 3

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

int size;
int nrGenerations;

std::unordered_set<Cell, Cell::hash> currentGeneration;
std::unordered_set<Cell, Cell::hash> nextGeneration;
std::unordered_map<Cell, int, Cell::hash> deadCells;

void evolve();
int getNeighbors(Cell cell);

inline void printResults();
inline void printCells(std::vector<Cell> &cells);
inline void printCells(std::unordered_set<Cell, Cell::hash> &cells);
inline void printCells(std::unordered_map<Cell, int, Cell::hash> &cells);

int main(int argc, char* argv[]) {

    if (argc != ARG_SIZE) {
        std::cout << "Usage: life3d <filename> <nr of generations>" << std::endl;
        return -1;
    }

    std::string filename = argv[1];
    nrGenerations = std::stoi(argv[2]);

    std::ifstream infile(filename);
    infile >> size;
 
    int x, y, z;

    while (infile >> x >> y >> z) {
        Cell cell(x, y, z);
        currentGeneration.insert(cell);
    }

    for (int i = 0; i < nrGenerations; i++) {
        evolve();
    }

    printResults();

    return 0;
}

void evolve() {

    for (auto it = currentGeneration.begin(); it != currentGeneration.end(); ++it) {
        int neighbors = getNeighbors(*it);
        
        if (neighbors >= 2 && neighbors <= 4) {
            // with 2 to 4 neighbors the cell lives 
            nextGeneration.insert(*it);
        }
    } 

    for (auto it = deadCells.begin(); it != deadCells.end(); ++it) {
        if (it->second == 2 || it->second == 3) {
            nextGeneration.insert(it->first);
        }
    }

    currentGeneration = nextGeneration; // new generation is our current generation
    nextGeneration = {}; // clears new generation
    deadCells.clear(); // clears dead cells from previous generation
}

int getNeighbors(Cell cell) {
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

    // x varies
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

    // y varies
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

    // z varies
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

    if (currentGeneration.count(cell1) > 0) {
        nrNeighbors++;
        // std::cout << "cell1" << std::endl;
    }
    else {
        deadCells[cell1] += 1;
        // std::cout << "dead cell1 neighbor: " << cell1 << std::endl;
    }
    if (currentGeneration.count(cell2) > 0) {
        nrNeighbors++;
        // std::cout << "cell2" << std::endl;
    }
    else {
        deadCells[cell2] += 1;
        // std::cout << "dead cell2 neighbor: " << cell2 << std::endl;
    }
    if (currentGeneration.count(cell3) > 0) {
        nrNeighbors++;  
        // std::cout << "cell3" << std::endl;
    }
    else {
        deadCells[cell3] += 1;
        // std::cout << "dead cell3 neighbor: " << cell3 << std::endl;
    }
    if (currentGeneration.count(cell4) > 0) {
        nrNeighbors++;
        // std::cout << "cell4" << std::endl;
    }
    else {
        deadCells[cell4] += 1;
        // std::cout << "dead cell4 neighbor: " << cell4 << std::endl;
    }
    if (currentGeneration.count(cell5) > 0) {
        nrNeighbors++;
        // std::cout << "cell5" << std::endl;
    }
    else {
        deadCells[cell5] += 1;
        // std::cout << "dead cell5 neighbor: " << cell5 << std::endl;
    }
    if (currentGeneration.count(cell6) > 0) {
        nrNeighbors++;
        // std::cout << "cell6" << std::endl;
    }
    else {
        deadCells[cell6] += 1;
        // std::cout << "dead cell6 neighbor: " << cell6 << std::endl;
    } 

    return nrNeighbors;
}

/* Aux functions for printing data */

inline void printResults() {
    std::set<Cell> lastGeneration(currentGeneration.begin(), currentGeneration.end());

    for (auto it = lastGeneration.begin(); it != lastGeneration.end(); ++it) {
        std::cout << *it << std::endl;
    }

    std::cout << "size: " << lastGeneration.size() << std::endl;
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
    for (auto it = deadCells.begin(); it != deadCells.end(); ++it) {
        std::cout << "cell: " << it->first << std::endl;
        std::cout << "nr: " << it->second << std::endl;
    }
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
