#include <iostream>
#include <chrono>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <omp.h>

using namespace std;

int read_decimal_number(fstream& file) {
    int number = 0;
    char digit;
    file.get(digit);
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
        cerr << "Usage: " << argv[0] << " <number_of_threads> <input_file_name> <output_file_name> <coeff>" << endl;
    } /*else {*/
        fstream inputfile(argv[2], ios::in | ios::binary);
        if (inputfile.is_open()) {
            unsigned char magic, filetype;
            inputfile >> magic >> filetype;
            if (magic != 'P' and (filetype != '5' or filetype != '6')) {
                std::cerr << "Not a PGM or PPM file" << std::endl;
            }

            inputfile.get();

            int width = read_decimal_number(inputfile);
            int height = read_decimal_number(inputfile);
            int maxval = read_decimal_number(inputfile);

            bool gray = filetype == '5';

            vector<unsigned char> picture (((gray) ? 1 : 3) * width * height);

            for (size_t i = 0; i < picture.size(); i++) {
                inputfile >> noskipws >> picture[i];
            }
            auto start = chrono::steady_clock::now();
            int coeff = width * height * atof(argv[4]);
            if (gray) {
                vector<int> color (256, 0);
                for (size_t i = 0; i < picture.size(); i++) {
                    color[picture[i]]++;
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
                    color[i] = ((float) max(0, (int)(i - min_color))) / (max_color - min_color) * 255;
                    if (color[i] > 255) {
                        color[i] = 255;
                    }
                }
                
                #pragma omp parallel for schedule (static, 64) 
                for (size_t i = 0; i < picture.size(); i++) {
                    picture[i] = color[picture[i]];
                   // picture[i] = min(255, (int) (((float) max(0, (int)(i - min_color))) / (max_color - min_color) * 255));
                }
            } else {
                vector<vector<int>> colors(3, vector<int> (256, 0));
                for (size_t i = 0; i < picture.size(); i++) {
                    colors[i % 3][picture[i]]++;
                }
                vector<unsigned char> max_colors (3);
                vector<unsigned char> min_colors (3);
                vector<int> count_max (3, 0);
                vector<int> count_min (3, 0);
                vector<bool> took_max (3, false);
                vector<bool> took_min (3, false);
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
                unsigned char max_color = max(max_colors[0], max(max_colors[1], max_colors[2]));
                unsigned char min_color = min(min_colors[0], min(min_colors[1], min_colors[2]));
                if (max_color == min_color) {
                    max_color = 255;
                    min_color = 0;
                }
            
                #pragma omp parallel for schedule (dynamic) 
                for (size_t i = 0; i < 256; i++) {
                    for (size_t j = 0; j < 3; j++) {
                        colors[j][i] = ((float) max(0, (int)(i - min_color))) / (max_color - min_color) * 255;
                        if (colors[j][i] > 255) {
                            colors[j][i] = 255;
                        }
                    }
                }
                
                #pragma omp parallel for schedule (dynamic)
                for (size_t i = 0; i < picture.size(); i++) {
                    picture[i] = colors[i % 3][picture[i]];
                   // picture[i] = min(255, (int) (((float) max(0, (int)(picture[i] - min_color))) / (max_color - min_color) * 255));
    
                }
            }
            auto end = chrono::steady_clock::now();
            chrono::duration<double> elapsed_ms = end - start;
            printf("Time (%i thread(s)): %g ms\n", atoi(argv[1]), elapsed_ms.count() * 1000);
            fstream outputfile(argv[3], ios::out | ios::binary);
            outputfile << magic << filetype << '\n' << width << " " << height << '\n' << maxval << '\n';
            for (size_t i = 0; i < picture.size(); i++) {
                unsigned char out = picture[i] & 0xFF;
                outputfile << picture[i];
            }
            outputfile.close();

        //}
    }
}
