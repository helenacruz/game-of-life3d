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
    struct hash;
    Cell(int x, int y, int z);
    std::vector<Cell> getNeighbors();

    friend std::ostream &operator<<(std::ostream &os, const Cell &cell);

    bool operator==(const Cell &rhs) const;

    bool operator!=(const Cell &rhs) const;

    bool isAlive() const;

    int getX() const;

    int getY() const;

    int getZ() const;
};


#endif //LIFE3D_CELL_H
