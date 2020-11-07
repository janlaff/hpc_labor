#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/*
Ja, weil die oberen und unteren zeilen des ghist layers auch innerhalb kommuniziert werden müssen,
Vorteil Buffer, einzelne Zeilen können einfach ausgetauscht werden.
Nachteil viel boilerplate code für jede richtung empfangen senden.
Vortail mpi datentyp saubere code, dafür schwiriger zu erstellen.
*/

#define calcCoord(x, width, numthreads) (((width)/(numthreads))*(x))

long TimeSteps = 3000;

int calcIndex(int row, int column, int height) {
    return row + (column * height);
}

void writeVTK2(long timestep, double *data, char prefix[1024], int coordX, int coordY, int w, int h) {
    char filename[2048];

    int offsetX = coordX * (w - 2);
    int offsetY = coordY * (h - 2);
    float deltax = 1.0;
    long nxy = (w - 2) * (h - 2) * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
    FILE * fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + (w - 2), offsetY , offsetY + (h - 2), 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char * ) & nxy, sizeof(long), 1, fp);

    for (int row = 1; row < h - 1; row++) {
        for (int col = 1; col < w - 1; col++) {
            float value = data[calcIndex(row, col, h)];
            fwrite((unsigned char * ) & value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void evolve(long t, double *currentfield, double *newfield, int w, int h) {
    for (int row = 1; row < h - 1; ++row) {
        for (int col = 1; col < w - 1; ++col) {
            double activeCount = 0;
            activeCount += currentfield[calcIndex(row - 1, col + 1, h)];
            activeCount += currentfield[calcIndex(row - 1, col - 1, h)];
            activeCount += currentfield[calcIndex(row - 1, col, h)];
            activeCount += currentfield[calcIndex(row, col + 1, h)];
            activeCount += currentfield[calcIndex(row, col - 1, h)];
            activeCount += currentfield[calcIndex(row + 1, col + 1, h)];
            activeCount += currentfield[calcIndex(row + 1, col - 1, h)];
            activeCount += currentfield[calcIndex(row + 1, col, h)];

            if (activeCount < 2 || activeCount > 3) {
                newfield[calcIndex(row, col, h)] = 0;
            } else if (activeCount == 2) {
                newfield[calcIndex(row, col, h)] = currentfield[calcIndex(row, col, h)];
            } else if (activeCount == 3) {
                newfield[calcIndex(row, col, h)] = 1;
            }
        }
    }
}

void filling(double * currentfield, int w, int h) {
    int i;
    for (i = 0; i < h * w; i++) {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }
}

int main(int argc, char ** argv) {
    int width = 120;
    int height = 120;

    int rank, size;
    MPI_Status status;

    // Initialize mpi
    MPI_Init( & argc, & argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Initialize random function
    srand((rank + 1) * clock());

    // Create topology
    int dims[1] = {size};
    int peri[1] = {1};
    MPI_Comm comm;
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, peri, 0, &comm);

    int leftRank, rightRank;
    MPI_Cart_shift(comm, 0, 1, &leftRank, &rightRank);

    int coord;
    MPI_Cart_coords(comm, rank, 1, &coord);

    int localX = coord;
    int localY = 0; // 1D
    int localWidth = (width / size) + 2;
    int localHeight = height + 2; // 1D

    double *currentfield = (double*)calloc(localWidth * localHeight, sizeof(double));
    double *newfield = (double*)calloc(localWidth * localHeight, sizeof(double));

    filling(currentfield, localWidth, localHeight);
    long t;
    for (t = 0; t < TimeSteps; t++) {
        if (rank % 2) {
            // Receive from left
            // GGGGG
            // X000G
            // X000G
            // GGGGG
            MPI_Recv(
                &currentfield[calcIndex(1, 0, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                leftRank,
                0,
                MPI_COMM_WORLD,
                &status
            );
            // Send to left
            // GGGGG
            // GX00G
            // GX00G
            // GGGGG
            MPI_Send(
                &currentfield[calcIndex(1, 1, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                leftRank,
                0,
                MPI_COMM_WORLD
            );
            // Send to right
            // GGGGG
            // G00XG
            // G00XG
            // GGGGG
            MPI_Send(
                &currentfield[calcIndex(1, localWidth - 2, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                rightRank,
                0,
                MPI_COMM_WORLD
            );
            // Receive from right
            // GGGGG
            // G000X
            // G000X
            // GGGGG
            MPI_Recv(
                &currentfield[calcIndex(1, localWidth - 1, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                rightRank,
                0,
                MPI_COMM_WORLD,
                &status
            );
        } else {
            // Send to right
            // GGGGG
            // G00XG
            // G00XG
            // GGGGG
            MPI_Send(
                &currentfield[calcIndex(1, localWidth - 2, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                rightRank,
                0,
                MPI_COMM_WORLD
            );
            // Receive from right
            // GGGGG
            // G000X
            // G000X
            // GGGGG
            MPI_Recv(
                &currentfield[calcIndex(1, localWidth - 1, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                rightRank,
                0,
                MPI_COMM_WORLD,
                &status
            );
            // Receive from left
            // GGGGG
            // X000G
            // X000G
            // GGGGG
            MPI_Recv(
                &currentfield[calcIndex(1, 0, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                leftRank,
                0,
                MPI_COMM_WORLD,
                &status
            );
            // Send to left
            // GGGGG
            // GX00G
            // GX00G
            // GGGGG
            MPI_Send(
                &currentfield[calcIndex(1, 1, localHeight)],
                localHeight - 2,
                MPI_DOUBLE,
                leftRank,
                0,
                MPI_COMM_WORLD
            );
        }

        // Set ghost layer
        for (int col = 0; col < localWidth; ++col) {
            currentfield[calcIndex(0, col, localHeight)] = currentfield[calcIndex(localHeight - 2, col, localHeight)];
            currentfield[calcIndex(localHeight - 1, col, localHeight)] = currentfield[calcIndex(1, col, localHeight)];
        }

        evolve(t, currentfield, newfield, localWidth, localHeight);

        char prefix[256] = {0};
        sprintf(prefix, "gol-%d", rank);
        writeVTK2(t, currentfield, prefix, localX, localY, localWidth, localHeight);

        printf("%ld timestep\n", t);

        //SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

    MPI_Finalize();

    return 0;
}