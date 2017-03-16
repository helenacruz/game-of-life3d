//
// Created by goncalo on 16-03-2017.
//

#ifndef LIFE3D_CELL_H
#define LIFE3D_CELL_H


#include <vector>
#include <ostream>

class Cell {

private:
    bool alive;
    int x;
    int y;
    int z;

public:
    Cell(int x, int y, int z);
    std::vector<Cell> getNeighbors();

    friend std::ostream &operator<<(std::ostream &os, const Cell &cell);

};


#endif //LIFE3D_CELL_H
