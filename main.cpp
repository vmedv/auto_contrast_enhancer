#include <algorithm>
#include <array>
#include <cstdint>
#include <ios>
#include <iostream>
#include <chrono>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <omp.h>

int read_decimal_number(std::fstream& file) {
  int number = 0;
  char digit;
  file.get(digit);
  while (iswspace(digit)) {
    file.get(digit);
  }
  while ('0' <= digit and digit <= '9') {
    number *= 10;
    number += digit - '0';
    file.get(digit);
  }
  return number;
}

int main(int argc, char *argv[]) {

    #ifdef _OPENMP
    omp_set_num_threads(atoi(argv[1]));
    #endif

  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " <number_of_threads> <input_file_name> <output_file_name> <coeff>" << std::endl;
    return 1;
  } 
  std::fstream inputfile(argv[2], std::ios::in | std::ios::binary);
  if (inputfile.is_open()) {
    unsigned char magic, filetype;
    inputfile >> magic >> filetype;
    if (magic != 'P' and (filetype != '5' or filetype != '6')) {
      std::cerr << "Not a PGM or PPM file" << std::endl;
    }

    int width = read_decimal_number(inputfile);
    int height = read_decimal_number(inputfile);
    int maxval = read_decimal_number(inputfile);

    bool gray = filetype == '5';

    std::array<std::vector<uint8_t>, 3> channels;
    channels[0] = std::vector<uint8_t>(width * height);
    if (!gray) {
      channels[1] = std::vector<uint8_t>(width * height);
      channels[2] = std::vector<uint8_t>(width * height);
    }

    for (size_t i = 0; i < width * height; i++) {
      inputfile >> std::noskipws >> channels[0][i];
      if (!gray) {
        inputfile >> std::noskipws >> channels[1][i];
        inputfile >> std::noskipws >> channels[2][i];
      }
    }
    auto start = std::chrono::steady_clock::now();
    int coeff = width * height * atof(argv[4]);
    if (gray) {
      std::array<int, 256> color;
      std::fill(color.begin(), color.end(), 0);
      for (size_t i = 0; i < width * height; i++) {
        color[channels[0][i]]++;
      }
      unsigned char max_color, min_color;
      int count_min = 0, count_max = 0;
      bool took_min = false, took_max = false;
      #pragma omp parallel for schedule (static, 64) 
      for (size_t i = 0; i < 256; i++) {
        count_min += color[i];
        count_max += color[255 - i];
        if (!took_min && count_min > coeff) {
          min_color = i;
          took_min = true;
        }
        if (!took_max && count_max > coeff) {
          max_color = 255 - i;
          took_max = true;
        }
      }
      if (max_color == min_color) {
        max_color = 255;
        min_color = 0;
      }

      #pragma omp parallel for schedule (static, 64) 
      for (size_t i = 0; i < 256; i++) {
        color[i] = ((float) std::max(0, (int)(i - min_color))) / (max_color - min_color) * 255;
        if (color[i] > 255) {
          color[i] = 255;
        }
      }

      #pragma omp parallel for schedule (static, 64) 
      for (size_t i = 0; i < width * height; i++) {
        channels[0][i] = color[channels[0][i]];
      }
    } else {
      std::array<std::array<int, 256>, 3> colors;
      #pragma omp parallel for 
      for (size_t i = 0; i < 3; i++) {
        colors[i] = {};
        std::fill(colors[i].begin(), colors[i].end(), 0);
      }
      #pragma omp parallel for schedule (static, 64)
      for (size_t i = 0; i < width * height; i++) {
        colors[0][channels[0][i]]++;
        colors[1][channels[1][i]]++;
        colors[2][channels[2][i]]++;
      }
      std::array<uint8_t, 3> max_colors = {0, 0, 0};
      std::array<uint8_t, 3> min_colors = {0, 0, 0};
      std::array<int, 3> count_max = {0, 0, 0};
      std::array<int, 3> count_min = {0, 0, 0};
      std::array<bool, 3> took_max = {false, false, false};
      std::array<bool, 3> took_min = {false, false, false};
      #pragma omp parallel for schedule (dynamic) 
      for (size_t i = 0; i < 256; i++) {
        for (size_t j = 0; j < 3; j++) {
          count_min[j] += colors[j][i];
          count_max[j] += colors[j][255 - i];
          if (!took_min[j] && count_min[j] > coeff) {
            min_colors[j] = i;
            took_min[j] = true;
          }
          if (!took_max[j] && count_max[j] > coeff) {
            max_colors[j] = 255 - i;
            took_max[j] = true;
          }
        }
      }
      unsigned char max_color = std::max(max_colors[0], std::max(max_colors[1], max_colors[2]));
      unsigned char min_color = std::min(min_colors[0], std::min(min_colors[1], min_colors[2]));
      if (max_color == min_color) {
        max_color = 255;
        min_color = 0;
      }

      #pragma omp parallel for schedule (dynamic) 
      for (size_t i = 0; i < 256; i++) {
        for (size_t j = 0; j < 3; j++) {
          colors[j][i] = ((float) std::max(0, (int)(i - min_color))) / (max_color - min_color) * 255;
          if (colors[j][i] > 255) {
            colors[j][i] = 255;
          }
        }
      }

      #pragma omp parallel for schedule (dynamic)
      for (size_t i = 0; i < width * height; i++) {
        channels[0][i] = colors[0][channels[0][i]];
        channels[1][i] = colors[1][channels[1][i]];
        channels[2][i] = colors[2][channels[2][i]];

      }
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_ms = end - start;
    printf("Time (%i thread(s)): %g ms\n", atoi(argv[1]), elapsed_ms.count() * 1000);
    std::fstream outputfile(argv[3], std::ios::out | std::ios::binary);
    outputfile << magic << filetype << '\n' << width << " " << height << '\n' << maxval << '\n';
    for (size_t i = 0; i < width * height; i++) {
      unsigned char out = channels[0][i] & 0xFF;
      outputfile << out;
      if (!gray) {
        out = channels[1][i] & 0xFF;
        outputfile << out;
        out = channels[2][i] & 0xFF;
        outputfile << out;
      }
    }
    outputfile.close();
  }
}
