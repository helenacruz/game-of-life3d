//
// Created by goncalo on 09-05-2017.
//
#include <set>
#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <unistd.h>

#define ARG_SIZE 3
#define NR_SETS 32
#define CHUNK 1
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
bool firstTimeRoot = true;
bool firstTimeOthers = true;
int *cellCounter, *deadCounter;


// Variables
typedef std::unordered_set<Cell, Cell::hash> CellSet;
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;
std::vector<CellSet> currentGeneration(NR_SETS);
std::vector<CellSet> nextGeneration(NR_SETS);
std::vector<DeadMap> deadCells(NR_SETS);
typedef std::unordered_map<Cell, int, Cell::hash> DeadMap;

// Function Headers
void distributeDeadCells();
void evolve();
void evolve(int n, int j);
int getNeighbors(Cell cell, int i);
inline int* getDataToSend();
inline int getSpaceCellSize(int j);
inline void prepareCellData(int* data, int count);
inline void initializeMap(std::vector<DeadMap> &maps);
void insertDeadCell(Cell cell);
void insertNextGeneration(Cell cell);
void prepareGeneration(int *data, int *offset);
inline void printResults();
int arraySize;

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
        int nCells = 0;
        while (infile >> x >> y >> z) {
            Cell cell(x, y, z);
            int index = cell.getIndex();
            currentGeneration[index].insert(cell);
            nCells++;

        }
    }

    // Initial Barrier
    elapsedTime = - MPI_Wtime();
    MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Initialize set counter array
    cellCounter = new int[nrProcesses];
    deadCounter = new int[nrProcesses];

    for(int i = 0; i < nrGenerations; i++){

        //MPI_Barrier (MPI_COMM_WORLD);
        if(!id) {
            if(firstTimeRoot){

                for (int j = 1; j < nrProcesses; j++) {
                    int *data = getDataToSend();

                    MPI_Send(data, arraySize, MPI_INT, j, OP_SEND_GENERATION, MPI_COMM_WORLD);
                }
                // all to all
                firstTimeRoot = false;
            }
            evolve(0, NR_SETS / nrProcesses);

            //evolve(0, (NR_SETS / nrProcesses) + 1);
            //evolve(0, NR_SETS / nrProcesses);

        }
        else{
            if(firstTimeOthers){

                // If all other non-root processes
                int count;

                // Probe for an incoming message from process zero
                MPI_Probe(0, OP_SEND_GENERATION, MPI_COMM_WORLD, &status);

                // When probe returns, the status object has the size and other
                // attributes of the incoming message. Get the message size
                MPI_Get_count(&status, MPI_INT, &count);

                // Allocate a buffer to hold the incoming numbers
                int data[count];

                MPI_Recv(data, count, MPI_INT, 0, OP_SEND_GENERATION, MPI_COMM_WORLD, &status);

                prepareCellData(data, count);

                // all to all
                firstTimeOthers = false;
            }

            evolve((NR_SETS / nrProcesses)*id, (NR_SETS /nrProcesses)*(id+1));

        }

        // First gather the size of each set among all processes to send
        int *dataToSend = getDataToSend();
        int dataSizeToSend = arraySize;
        MPI_Allgather(&dataSizeToSend, 1, MPI_INT, cellCounter, 1, MPI_INT, MPI_COMM_WORLD);

        // Then ...
        // Allocate data for the receiving array
        int totalSizeToReceive = 0;
        for(int m = 0; m < nrProcesses; m++){
            totalSizeToReceive += cellCounter[m];
        }
        int receivedData[totalSizeToReceive];

        // Get the offset of each process
        int offset[nrProcesses];
        offset[0] = 0;
        for (int j = 1; j < nrProcesses; j++) {
            offset[j] = offset[j - 1] + cellCounter[j - 1];
        }

        MPI_Allgatherv(dataToSend, dataSizeToSend, MPI_INT, receivedData, cellCounter, offset ,MPI_INT, MPI_COMM_WORLD);
        prepareGeneration(receivedData, offset);

    }

    if(!id){
        printResults();
    }



    // Final Barrier
    MPI_Barrier (MPI_COMM_WORLD);
    elapsedTime += MPI_Wtime();



    MPI_Finalize();

    return 0;
}

inline int* getDataToSend(){

    // data do send space declaration
    int initialSetIndex = 0;
    int finalSetIndex = NR_SETS; // Index for j to process are [initialSetIndex , finalSetIndex]
    int spaceCellSets = 0;

    for(int i = initialSetIndex; i < finalSetIndex; i++){
        spaceCellSets+=currentGeneration[i].size();
    }

    int sizeArray = 3*spaceCellSets + (NR_SETS - 1);
    arraySize = sizeArray;


    int *data = new int[sizeArray];


    int index = 0;

    // Iterate vector through indexes to send
    for(int i = initialSetIndex; i < finalSetIndex; i++){
        CellSet &set = currentGeneration[i];

        // Iterate set
        for (auto it = set.begin(); it != set.end(); ++it) {
            Cell cell = *it;
            data[index] = cell.getX();
            data[index+1] = cell.getY();
            data[index+2] = cell.getZ();

            index+=3;
        }

        // When we end processing a set, put a border marker
        if(index != sizeArray)
            data[index] = -1;

        index++;

    }

    return data;
}

/**
 * counter: vector of numbers of elements each process sends
 * displs: vector that contains the index where each process data starts
 */
inline void prepareGeneration(int *data, int *offset){
    int nrBorders;
    int index = 0;

    for (int i = 0; i < nrProcesses; i++) {
        nrBorders = 0;
        for (int j = offset[i]; j < cellCounter[i] + offset[i]; ) {
            if(id== 0){
              // std::cout << "index: " << index << std::endl;
            }
            index = nrBorders % NR_SETS;
            if (data[j] == -1) {
                nrBorders++;
                j++;
            }
            else {
                Cell cell(data[j], data[j + 1], data[j + 2]);
                currentGeneration[index].insert(cell);
                j += 3;
            }
        }
    }
}

inline void prepareCellData(int *data, int count){

    int nrBorders = 0;
    for(int i = 0; i< count; ){
        int value = data[i];
        if(value == -1){
            nrBorders++;
            i++;
        }else{
            Cell cell(data[i], data[i+1], data[i+2]);
            currentGeneration[nrBorders].insert(cell);
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

void evolve(int initial, int end) {
    if (id == nrProcesses - 1) { // last process does the rest
        end = NR_SETS;
    }
    #pragma omp parallel
    {
        // We will divide the current generation vector sets dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK)
        for (int i = initial; i < end; i++) {
            // Each thread iterates through a set...
            CellSet &set = currentGeneration[i];

            for (auto it = set.begin(); it != set.end(); ++it) {
                int neighbors = getNeighbors(*it, i);

                Cell cell = *it;
                if (neighbors >= 2 && neighbors <= 4) {
                    // with 2 to 4 neighbors the cell lives
                    insertNextGeneration(*it);
                }
            }
        }
    }

    distributeDeadCells();

    #pragma omp parallel
    {
        // We will also divide the dead cells map dynamically among various threads available
        #pragma omp for schedule(dynamic, CHUNK)
        for (int i = 0; i < NR_SETS; i++) {
            // Each thread iterates through a map
            DeadMap &map = deadCells[i];

            for (auto it = map.begin(); it != map.end(); ++it){

                //std::cout << it->first << "      " << "n: " << it->second << std::endl;
               // fflush(stdout);
              //  usleep(100000);

                if (it->second == 2 || it->second == 3) {
                    insertNextGeneration(it->first);
                    Cell cell = it->first;
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

void distributeDeadCells(){
    int deadNr = 0;
    for (int i = 0; i < NR_SETS; i++) {
        // Each thread iterates through a map
        DeadMap &map = deadCells[i];

        deadNr += map.size();

    }
    deadNr = deadNr*4;
    int deadArray[deadNr];

    int index = 0;
    //std::cout << "=================" << std::endl;
    for (int i = 0; i < NR_SETS; i++) {
        DeadMap &map = deadCells[i];
        for (auto it = map.begin(); it != map.end(); ++it){
            deadArray[index] = it->first.getX();
            deadArray[index+1] = it->first.getY();
            deadArray[index+2] = it->first.getZ();
            deadArray[index+3] = it->second;
            index+=4;

        }

    }

    // First gather the size of each set among all processes to send

    MPI_Allgather(&deadNr, 1, MPI_INT, deadCounter, 1, MPI_INT, MPI_COMM_WORLD);

    // Then ...
    // Allocate data for the receiving array
    int totalSizeToReceive = 0;
    for(int m = 0; m < nrProcesses; m++){
        totalSizeToReceive += deadCounter[m];
    }
    int receivedData[totalSizeToReceive];

    // Get the offset of each process
    int offset[nrProcesses];
    offset[0] = 0;
    for (int j = 1; j < nrProcesses; j++) {
        offset[j] = offset[j - 1] + deadCounter[j - 1];
    }

    MPI_Allgatherv(deadArray, deadNr, MPI_INT, receivedData, deadCounter, offset ,MPI_INT, MPI_COMM_WORLD);


    for (int i = 0; i < nrProcesses; i++) {
        if (id == i) {
            continue;
        }
        for (int j = offset[i]; j < deadCounter[i] + offset[i]; j+=4) {
            Cell cell(receivedData[j], receivedData[j + 1], receivedData[j + 2]);
            deadCells[cell.getIndex()][cell] += receivedData[j+3];
            //std::cout << "cell: " << cell << " nr: " << receivedData[j+3]<< " contador: " << deadCells[cell.getIndex()][cell] << std::endl;
            usleep(10000);
        }
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

inline void initializeMap(std::vector<DeadMap> &maps) {
    for (int i = 0; i < NR_SETS; i++) {
        DeadMap map;
        maps[i] = map;
    }
}

/* Aux functions for printing data */
inline void printResults() {
    std::set<Cell> lastGeneration;

    for (int i = 0; i < NR_SETS; i++) {
       //std::cout << currentGeneration[i].size() << std::endl;
        lastGeneration.insert(currentGeneration[i].begin(),
                              currentGeneration[i].end());
    }

    for (auto it = lastGeneration.begin(); it != lastGeneration.end(); ++it) {
        std::cout << *it << std::endl;
    }
}