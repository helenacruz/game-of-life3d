//
// Created by goncalo on 16-03-2017.
//

#include "Cell.h"

Cell::Cell(int x, int y, int z) : x(x), y(y), z(z) {

}

std::ostream &operator<<(std::ostream &os, const Cell &cell) {
    os << "x: " << cell.x << " y: " << cell.y << " z: " << cell.z;
    return os;
}

std::vector<Cell> Cell::getNeighbors() {
    return std::vector<Cell>();
}


bool Cell::operator==(const Cell &rhs) const {
    return alive == rhs.alive &&
           x == rhs.x &&
           y == rhs.y &&
           z == rhs.z;
}

bool Cell::operator!=(const Cell &rhs) const {
    return !(rhs == *this);
}

bool Cell::isAlive() const {
    return alive;
}

int Cell::getX() const {
    return x;
}

int Cell::getY() const {
    return y;
}

int Cell::getZ() const {
    return z;
}


struct hash
{
    size_t operator()(Cell const & x) const noexcept
    {
        return (
                 (51 + std::hash<int>()(x.getX())) *
                 (51 + std::hash<int>()(x.getY())) *
                 (51 + std::hash<int>()(x.getZ()))
        );
    }
};

