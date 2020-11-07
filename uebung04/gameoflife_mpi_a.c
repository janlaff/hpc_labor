#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x, y)((y) * (width) + (x))

long TimeSteps = 100;

void writeVTK2(long timestep, double * data, char prefix[1024], int w, int h) {
    char filename[2048];
    int x, y;

    int offsetX = 0;
    int offsetY = 0;
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

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            float value = data[calcIndex(h, x, y)];
            fwrite((unsigned char * ) & value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

double getCellValue(int x, int y, int w, int h, double * currentField) {
    if (x < 0) x = w - 1;
    else if (x >= w) x = 0;
    if (y < 0) y = h - 1;
    else if (y >= h) y = 0;

    return currentField[calcIndex(w, x, y)];
}

void evolve(double * currentfield, double * newfield, int w, int h) {
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {

            double activeCount = 0;
            activeCount += getCellValue(x - 1, y, w, h, currentfield);
            activeCount += getCellValue(x + 1, y, w, h, currentfield);
            activeCount += getCellValue(x - 1, y + 1, w, h, currentfield);
            activeCount += getCellValue(x + 1, y + 1, w, h, currentfield);
            activeCount += getCellValue(x, y + 1, w, h, currentfield);
            activeCount += getCellValue(x - 1, y - 1, w, h, currentfield);
            activeCount += getCellValue(x + 1, y - 1, w, h, currentfield);
            activeCount += getCellValue(x, y - 1, w, h, currentfield);

            if (activeCount < 2 || activeCount > 3) {
                newfield[calcIndex(w, x, y)] = 0;
            } else if (activeCount == 2) {
                newfield[calcIndex(w, x, y)] = currentfield[calcIndex(w, x, y)];
            } else if (activeCount == 3) {
                newfield[calcIndex(w, x, y)] = 1;
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

void game(int w, int h) {
    double * currentfield = calloc(w * h, sizeof(double));
    double * newfield = calloc(w * h, sizeof(double));

    filling(currentfield, w, h);
    long t;
    for (t = 0; t < TimeSteps; t++) {
        evolve(currentfield, newfield, w, h);

        printf("%ld timestep\n", t);
        writeVTK2(t, currentfield, "gol", w, h);

        usleep(200000);

        //SWAP
        double * temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

}

int main(int argc, char ** argv) {
    game(30, 30);

    return 0;
}