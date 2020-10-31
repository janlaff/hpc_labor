#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <math.h>

#include <omp.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))


static int numRows = 10, numColumns = 10;

long TimeSteps = 100;

void writeVTK2(long timestep, double *data, char prefix[1024], int offX, int offY, int w, int h, int fieldWidth) {
  char filename[2048];  
  int x,y; 
  
  int offsetX=offX;
  int offsetY=offY;
  float deltax=1.0;
  long  nxy = w * h * sizeof(float);  

  snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
  FILE* fp = fopen(filename, "w");

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
  fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
  fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp, "</CellData>\n");
  fprintf(fp, "</ImageData>\n");
  fprintf(fp, "<AppendedData encoding=\"raw\">\n");
  fprintf(fp, "_");
  fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

  for (y = offsetY; y < offsetY + h; y++) {
    for (x = offsetX; x < offsetX + w; x++) {
      float value = data[calcIndex(fieldWidth, x,y)];
      fwrite((unsigned char*)&value, sizeof(float), 1, fp);
    }
  }
  
  fprintf(fp, "\n</AppendedData>\n");
  fprintf(fp, "</VTKFile>\n");
  fclose(fp);
}
void show(double* currentfield, int w, int h) {
  printf("\033[H");
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    // printf("\033[E");
    printf("\n");
  }
  fflush(stdout);
}
 
 double getCellValue(int x, int y, int w, int h, double* currentField)
 {
   if(x < 0) x = w - 1;
   else if(x >= w) x = 0;
   if(y < 0) y = h - 1;
   else if(y >= h) y = 0;

   return currentField[calcIndex(w, x, y)];
 }


void evolve(long t, double* currentfield, double* newfield, int w, int h) {

  #pragma omp parallel num_threads(numRows * numColumns)
  {
    int this_thread = omp_get_thread_num();

    int rectWidth = w / numColumns;
    int rectHeight = h / numRows;

    int yIndex = this_thread * rectWidth / w;

    int startingPositionX = (this_thread * rectWidth) % w;
    int startingPositionY =  rectHeight * yIndex;

    for (int x = startingPositionX; x < startingPositionX + rectWidth; x++) {
      for (int y = startingPositionY; y < startingPositionY + rectHeight; y++) {

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
        }
        else if (activeCount == 2) {
          newfield[calcIndex(w, x, y)] = currentfield[calcIndex(w, x, y)];
        }
        else if (activeCount == 3) {
          newfield[calcIndex(w, x, y)] = 1;
        }
      }
    }

    char prefix[256] = {0};
    sprintf(prefix, "gol-%d", this_thread);

    writeVTK2(t,currentfield, prefix, startingPositionX, startingPositionY, rectWidth, rectHeight, w);
  }
}
 
void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}
 
void game(int w, int h) {
  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));
  
  //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));
  
  filling(currentfield, w, h);
  long t;
  for (t=0;t<TimeSteps;t++) {
    show(currentfield, w, h);
    evolve(t, currentfield, newfield, w, h);
    
    printf("%ld timestep\n",t);
    
    usleep(200000);

    //SWAP
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
  
}
 
int main(int c, char **v) {
  int w = 0, h = 0;
  if (c > 1) w = atoi(v[1]); ///< read width
  if (c > 2) h = atoi(v[2]); ///< read height
  if (c > 3) w = atoi(v[3]); ///< thread num x
  if (c > 4) h = atoi(v[4]); ///< thread num y
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height
  game(w, h);
}
