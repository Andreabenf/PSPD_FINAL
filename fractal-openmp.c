#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#define OUTFILE "out_julia_normal.bmp"

int compute_julia_pixel(int x, int y, int largura, int altura, float tint_bias, unsigned char *rgb) {
  // Check coordinates
  if ((x < 0) || (x >= largura) || (y < 0) || (y >= altura)) {
    fprintf(stderr,"Invalid (%d,%d) pixel coordinates in a %d x %d image\n", x, y, largura, altura);
    return -1;
  }
  // "Zoom in" to a pleasing view of the Julia set
  float X_MIN = -1.6, X_MAX = 1.6, Y_MIN = -0.9, Y_MAX = +0.9;
  float float_y = (Y_MAX - Y_MIN) * (float)y / altura + Y_MIN ;
  float float_x = (X_MAX - X_MIN) * (float)x / largura  + X_MIN ;
  // Point that defines the Julia set
  float julia_real = -.79;
  float julia_img = .15;
  // Maximum number of iteration
  int max_iter = 300;
  // Compute the complex series convergence
  float real=float_y, img=float_x;
  int num_iter = max_iter;
  while (( img * img + real * real < 2 * 2 ) && ( num_iter > 0 )) {
    float xtemp = img * img - real * real + julia_real;
    real = 2 * img * real + julia_img;
    img = xtemp;
    num_iter--;
  }

  // Paint pixel based on how many iterations were used, using some funky colors
  float color_bias = (float) num_iter / max_iter;
  rgb[0] = (num_iter == 0 ? 200 : - 500.0 * pow(tint_bias, 1.2) *  pow(color_bias, 1.6));
  rgb[1] = (num_iter == 0 ? 100 : -255.0 *  pow(color_bias, 0.3));
  rgb[2] = (num_iter == 0 ? 100 : 255 - 255.0 * pow(tint_bias, 1.2) * pow(color_bias, 3.0));

  return 0;
} /*fim compute julia pixel */

int write_bmp_header(FILE *f, int largura, int altura) {

  unsigned int row_size_in_bytes = largura * 3 +
	  ((largura * 3) % 4 == 0 ? 0 : (4 - (largura * 3) % 4));

  // Define all fields in the bmp header
  char id[2] = "BM";
  unsigned int filesize = 54 + (int)(row_size_in_bytes * altura * sizeof(char));
  short reserved[2] = {0,0};
  unsigned int offset = 54;

  unsigned int size = 40;
  unsigned short planes = 1;
  unsigned short bits = 24;
  unsigned int compression = 0;
  unsigned int image_size = largura * altura * 3 * sizeof(char);
  int x_res = 0;
  int y_res = 0;
  unsigned int ncolors = 0;
  unsigned int importantcolors = 0;

  // Write the bytes to the file, keeping track of the
  // number of written "objects"
  size_t ret = 0;
  ret += fwrite(id, sizeof(char), 2, f);
  ret += fwrite(&filesize, sizeof(int), 1, f);
  ret += fwrite(reserved, sizeof(short), 2, f);
  ret += fwrite(&offset, sizeof(int), 1, f);
  ret += fwrite(&size, sizeof(int), 1, f);
  ret += fwrite(&largura, sizeof(int), 1, f);
  ret += fwrite(&altura, sizeof(int), 1, f);
  ret += fwrite(&planes, sizeof(short), 1, f);
  ret += fwrite(&bits, sizeof(short), 1, f);
  ret += fwrite(&compression, sizeof(int), 1, f);
  ret += fwrite(&image_size, sizeof(int), 1, f);
  ret += fwrite(&x_res, sizeof(int), 1, f);
  ret += fwrite(&y_res, sizeof(int), 1, f);
  ret += fwrite(&ncolors, sizeof(int), 1, f);
  ret += fwrite(&importantcolors, sizeof(int), 1, f);

  // Success means that we wrote 17 "objects" successfully
  return (ret != 17);
} /* fim write bmp-header */

int main(int argc, char *argv[]) {
    int n;
    int area=0, largura = 0, altura = 0, local_i= 0;
    FILE *output_file;
    unsigned char *pixel_array, *rgb;

    if ((argc <= 1)|(atoi(argv[1])<1)){
        fprintf(stderr,"Entre 'N' como um inteiro positivo! \n");
        return -1;
    }

    n = atoi(argv[1]);
    altura = n;
    largura = 2 * n;
    area = altura * largura * 3;

    // Allocate memory for the pixels array
    pixel_array = calloc(area, sizeof(unsigned char));
    rgb = calloc(3, sizeof(unsigned char));

    printf("Computando linhas de pixel %d até %d, para uma área total de %d\n", 0, n-1, area);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int num_threads = omp_get_num_threads();
        int local_altura = altura / num_threads;
        int local_offset = tid * local_altura;

        unsigned char *local_pixel_array = calloc(local_altura * largura * 3, sizeof(unsigned char));

        for (int i = local_offset; i < local_offset + local_altura; i++) {
            for (int j = 0; j < largura * 3; j += 3) {
                compute_julia_pixel(j / 3, i, largura, altura, 1.0, rgb);
                local_pixel_array[(i - local_offset) * largura * 3 + j] = rgb[0];
                local_pixel_array[(i - local_offset) * largura * 3 + j + 1] = rgb[1];
                local_pixel_array[(i - local_offset) * largura * 3 + j + 2] = rgb[2];
            }
        }

        #pragma omp critical
        {
            for (int i = local_offset; i < local_offset + local_altura; i++) {
                for (int j = 0; j < largura * 3; j += 3) {
                    pixel_array[i * largura * 3 + j] = local_pixel_array[(i - local_offset) * largura * 3 + j];
                    pixel_array[i * largura * 3 + j + 1] = local_pixel_array[(i - local_offset) * largura * 3 + j + 1];
                    pixel_array[i * largura * 3 + j + 2] = local_pixel_array[(i - local_offset) * largura * 3 + j + 2];
                }
            }
        }

        free(local_pixel_array);
    }

    // Release memory for the rgb array
    free(rgb);

    // Write the header of the output file
    output_file = fopen(OUTFILE, "w");
    write_bmp_header(output_file, largura, altura);

    // Write the pixel array to the output file
    fwrite(pixel_array, sizeof(unsigned char), area, output_file);
    fclose(output_file);

    // Release memory for the pixel array
    free(pixel_array);

    return 0;
}
