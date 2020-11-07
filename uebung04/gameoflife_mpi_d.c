#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define calcCoord(x, width, numthreads) (((width)/(numthreads))*(x))

int calcIndex(int row, int column, int height) {
    return row + (column * height);
}

void writeVTK2(long timestep, double *data, char prefix[1024], int coordX, int coordY, int w, int h) {
    char filename[2048];

    int offsetX = coordX * (w - 2);
    int offsetY = coordY * (h - 2);
    float deltax = 1.0;
    long nxy = (w - 2) * (h - 2) * sizeof(float);

    printf("Xoff: %d, Yoff: %d\n", offsetX, offsetY);

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

int main(int argc, char ** argv) {
    int width = 30;
    int height = 10;

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

    int coord;
    MPI_Cart_coords(comm, rank, 1, &coord);

    int leftCoord, rightCoord;
    MPI_Cart_coords(comm, left, 1, &leftCoord);
    MPI_Cart_coords(comm, right, 1, &rightCoord);

    int localX = coord;
    int localY = 0; // 1D
    int localWidth = (width / size) + 2;
    int localHeight = height + 2; // 1D

    double *data = (double*)calloc(localWidth * localHeight, sizeof(double));
    printf("Rank: %d, X: %d, Y: %d, W: %d, H: %d\n", rank, localX, localY, localWidth, localHeight);

    char prefix[256] = {0};
    sprintf(prefix, "gol-%d", rank);

    writeVTK2(0, data, prefix, localX, localY, localWidth, localHeight);

    free(data);
    MPI_Finalize();

    return 0;
}