#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char ** argv) {
    int rank, size, i;
    MPI_Status status;

    // Initialize mpi
    MPI_Init( & argc, & argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Create topology
    int dims[1] = {size};
    int peri[1] = {1};
    MPI_Comm comm;
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, peri, 0, &comm);

    int left, right;
    MPI_Cart_shift(comm, 0, 1, &left, &right);

    printf("Rank: %d, Left: %d, Right: %d\n", rank, left, right);

    MPI_Finalize();

    return 0;
}