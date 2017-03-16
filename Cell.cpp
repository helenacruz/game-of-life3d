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
