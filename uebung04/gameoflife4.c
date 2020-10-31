#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int calcIndex(int row, int column, int height) {
    return row + (column * height);
}

int main(int argc, char ** argv) {
    int rank, size, i;
    MPI_Status status;

    MPI_Init( & argc, & argv);
    MPI_Comm_size(MPI_COMM_WORLD, & size);
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);

    int width = 30;
    int height = 30;
    // Vertical sections divided in x axis
    int localWidth = width / size;
    // Height stays the same
    int localHeight = height;
    // Cells with 1 cell border on each side
    int localCellCount = (localWidth + 2) * (localHeight + 2);
    // Buffer containing the cell data
    double* cellBuf = (double*)calloc(localCellCount, sizeof(double));

    // Evolve

    if (rank > 0) {
        // Receive from left
        // GGGGG
        // X000G
        // X000G
        // GGGGG
        MPI_Recv(
            &cellbuf[calcIndex(1, 0, localHeight)],
            localHeight - 2,
            rank - 1,
            0,
            MPI_COMM_WORLD
        );
        // Send to left
        // GGGGG
        // GX00G
        // GX00G
        // GGGGG
        MPI_Send(
            &cellBuf[calcIndex(1, 1, localHeight)],
            localHeight - 2,
            MPI_DOUBLE,
            rank - 1,
            0,
            MPI_COMM_WORLD
        );
    }

    if (rank < size - 1) {
        // Send to right
        // GGGGG
        // G00XG
        // G00XG
        // GGGGG
        MPI_Send(
            &cellBuf[calcIndex(1, localWidth - 2, localHeight)],
            localHeight - 2,
            MPI_DOUBLE,
            rank + 1,
            0,
            MPI_COMM_WORLD
        );
        // Receive from right
        // GGGGG
        // G000X
        // G000X
        // GGGGG
        MPI_Recv(
            &cellbuf[calcIndex(1, localWidth - 1, localHeight)],
            localHeight - 2,
            rank + 1,
            0,
            MPI_COMM_WORLD
        );
    }

    MPI_Finalize();

    return 0;
}