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
#define OP_SEND_GENERATION 1

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

int size, nrProcesses, id;

inline int* getDataToSend(int j);
inline int getSpaceCellSize(int j);

typedef std::unordered_set<Cell, Cell::hash> CellSet;
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;
std::vector<CellSet> currentGeneration(NR_SETS);
inline void prepareCellData(int* data, int count);

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


    if(!id) {
        // If the root process
        // We have to broadcast first to all other processes, their corresponding cell sets, with corresponding indexes

       // for (int i = 0; i < nrGenerations; i++) {

            for (int j = 1; j < nrProcesses; j++) {
                int *data = getDataToSend(j);

                MPI_Send(data, getSpaceCellSize(j), MPI_INT, j, OP_SEND_GENERATION, MPI_COMM_WORLD);

            }
        //}

    }
    else{
        // If all other non-root processes
        int count;

        // Probe for an incoming message from process zero
        MPI_Probe(0, OP_SEND_GENERATION, MPI_COMM_WORLD, &status);

        // When probe returns, the status object has the size and other
        // attributes of the incoming message. Get the message size
        MPI_Get_count(&status, MPI_INT, &count);

        // Allocate a buffer to hold the incoming numbers
        int* data = new int[count];

        MPI_Recv(data, count, MPI_INT, 0, OP_SEND_GENERATION, MPI_COMM_WORLD, &status);

        prepareCellData(data, count);

        // When not needed anymore, free data
        free(data);
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

/**
 * Returns an int array that follows following index structure:
 *
 * @param j process id to which send data
 * @return
 */

inline int getSpaceCellSize(int j){
    int nrSets = NR_SETS / nrProcesses;
    int spaceCellSets = 0;
    int initialSetIndex = j * nrSets;
    int finalSetIndex = (j+1)*nrSets -1; // Index for j to process are [initialSetIndex , finalSetIndex]

    for(int i = initialSetIndex; i <= finalSetIndex; i++){
        spaceCellSets+=currentGeneration[i].size();
    }

    return spaceCellSets;
}

inline int* getDataToSend(int j){

    // data do send space declaration
    int nrSets = NR_SETS / nrProcesses;
    int initialSetIndex = j * nrSets;
    int finalSetIndex = (j+1)*nrSets -1; // Index for j to process are [initialSetIndex , finalSetIndex]
    int spaceCellSets = 0;

    for(int i = initialSetIndex; i <= finalSetIndex; i++){
        spaceCellSets+=currentGeneration[i].size();
    }


    // 1 for initial index + 1 for number of cellsets + Space for cell sets
    // + 1 for each set so we can put border number, like -1
    // so the receiving side can diferentiate when one set ends and the other starts
    int sizeArray = spaceCellSets + 2 + (finalSetIndex - initialSetIndex);

    int* data = new int[sizeArray];

    // data to send definition
    data[0] = initialSetIndex;
    data[1] = (finalSetIndex - initialSetIndex) + 1;


    int index = 2;
    int which = 0; // purpose is to differentiate between x, y , z

    // Iterate vector through indexes to send
    for(int i = initialSetIndex; i < finalSetIndex; i++){
        CellSet &set = currentGeneration[i];

        // Iterate set
        for (auto it = set.begin(); it != set.end(); ++it) {

            Cell cell = *it;
            while(which < 3){
                switch (which){
                    case 0:
                        data[index] = cell.getX();
                        break;
                    case 1:
                        data[index] = cell.getY();
                        break;
                    case 2:
                        data[index] = cell.getZ();
                        break;
                }
                which++;
            }
            index++;
            which = 0;
        }

        // When we end processing a set, put a border marker
        data[index] = -1;
        index++;

    }
    return data;
}

inline void prepareCellData(int *data, int count){
    int initialIndex = data[0];
    int finalIndex = data[1]; //TODO: it's not needed remove from communication transit

    int nrElements = count - 2;
    int nrBorders = 0;
    int index;
    for(int i = 2; i< nrElements + 2;){
        index = initialIndex + nrBorders;
        int number = data[i];
        if(number == -1){
            // It's a border marker
            // Time to pass on to next set
            nrBorders++;
            i++;
        }else{
            Cell cell(data[i], data[i+1], data[i+2]);
            currentGeneration[index].insert(cell);
            i+=3;
        }
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