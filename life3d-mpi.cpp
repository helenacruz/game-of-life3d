//
// Created by goncalo on 09-05-2017.
//
#include <mpi.h>
#include <cstdlib>
#include <iostream>


#define TAG 123

int main(int argc, char* argv[]) {
    MPI_Status status;

    int id, nrProcesses;
    double elapsedTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nrProcesses); //Number of processes is on nprocs
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // Number of current process

    // Initial Barrier
    MPI_Barrier (MPI_COMM_WORLD);
    elapsedTime = - MPI_Wtime();

    // Program Execution


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