//
// Created by goncalo on 09-05-2017.
//
#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#define ARG_SIZE 3
#define NR_SETS 32
#define CHUNK 4

inline int generateIndex(int x, int y, int z);

int size;


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

typedef std::unordered_set<Cell, Cell::hash> CellSet;
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;
std::vector<CellSet> currentGeneration(NR_SETS);

class Data{
    int index;
    std::vector<CellSet> generation;

public:
    Data(int index, std::vector<CellSet> generation) : index(index), generation(generation) {}
};

int main(int argc, char* argv[]) {

    // Argument reading
    if (argc != ARG_SIZE) {
        std::cout << "Usage: life3d <filename> <nr of generations>" << std::endl;
        return -1;
    }
    std::string filename = argv[1];
    int nrGenerations = std::stoi(argv[2]);

    // Initial configuration
    MPI_Status status;

    int id, nrProcesses;
    double elapsedTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nrProcesses); //Number of processes is on nprocs
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // Number of current process

    // File reading
    // Only launching process reads configuration file
    // And prepares initial generation
    if(!id){
        std::ifstream infile(filename);
        infile >> size;
        int x, y, z;


        while (infile >> x >> y >> z) {
            Cell cell(x, y, z);
            currentGeneration[cell.getIndex()].insert(cell);
        }
    }


    // Initial Barrier
    MPI_Barrier (MPI_COMM_WORLD);
    elapsedTime = - MPI_Wtime();


    // Program Execution
    Data data = Data(0, currentGeneration);
    size_t buffer_size = sizeof(int) + sizeof(currentGeneration);
    char *buffer = new char[buffer_size];

    //MPI_Bcast(&data, sizeof(data), MPI_BYTE,, )

    for (int i = 0; i < nrGenerations; i++) {
        //evolve();
    }


    // Final Barrier
    MPI_Barrier (MPI_COMM_WORLD);
    elapsedTime += MPI_Wtime();


    std::cout << "Total execution time: " << elapsedTime << std::endl;

    MPI_Finalize();

    return 0;
}
/**
 *  Don't forget:
 *
 *  MPI_SEND(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
 *  MPI_SEND is asynchronous
 *  -> dest must be a valid rank between 0,...,N-1
 *
 *  MPI_RECV(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
 *  MPI_RECV is synchronous, i.e, process will lock till it receives something
 *
 *  MPI_Barrier (MPI_COMM_WORLD);
 *  MPI_Barrier is an active waiting for all processes to reach this point
 *
 *  MPI_ANY_SOURCE -> disregards the id of the sending process
 *                 -> i.e, accepts a write from any process
 *
 *  MPI_ANY_TAG -> on the sending side, a TAG must be specified (int)
 *              -> on the receiving side, a TAG must also be specified, but
 *              wildcard MPI_ANY_TAG matches any tag.
 *
 *  Reasons to use wildcards:
 *  - receiving messages from several sources into the same buffer (use MPI_ANY_SOURCE)
 *  - receiving several messages from the same source into the same buffer, and don’t care about the order
 *  (use MPI ANY TAG)
 *
 *  MPI Status is a structure on the receiving side, with the following attributes:
 *  - status.MPI TAG is tag of incoming message
 *  - status.MPI SOURCE is source of incoming message
 *  - MPI Get count(IN status, IN datatype, OUT count) -> how many elements of a given datatype were received
 *
 *
 */

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