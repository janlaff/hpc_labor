#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

void writeVTK2(long timestep, double * data, char prefix[1024], int offX, int offY, int w, int h, int fieldWidth) {
    char filename[2048];
    int x, y;

    int offsetX = offX;
    int offsetY = offY;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
    FILE * fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char * ) & nxy, sizeof(long), 1, fp);

    for (y = offsetY; y < offsetY + h; y++) {
        for (x = offsetX; x < offsetX + w; x++) {
            float value = data[calcIndex(fieldWidth, x, y)];
            fwrite((unsigned char * ) & value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

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

    int coord;
    MPI_Cart_coords(comm, rank, 1, &coord);

    double data[] = {0.0, 0.0, 1.0, 0.0};
    char prefix[256] = {0};
    sprintf(prefix, "gol-%d", rank);

    writeVTK2(0, &data, prefix, 2 * coord, 0, 2, 2, 2 * size);

    MPI_Finalize();

    return 0;
}