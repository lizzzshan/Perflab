#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int main(int argc, char **argv)
{
    
    if ( argc < 2) {
        fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
    }
    
    //
    // Convert to C++ strings to simplify manipulation
    //
    string filtername = argv[1];
    
    //
    // remove any ".filter" in the filtername
    //
    string filterOutputName = filtername;
    string::size_type loc = filterOutputName.find(".filter");
    if (loc != string::npos) {
        //
        // Remove the ".filter" name, which should occur on all the provided filters
        //
        filterOutputName = filtername.substr(0, loc);
    }
    
    Filter *filter = readFilter(filtername);
    
    double sum = 0.0;
    int samples = 0;
    
    for (int inNum = 2; inNum < argc; inNum++) {
        string inputFilename = argv[inNum];
        string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
        struct cs1300bmp *input = new struct cs1300bmp;
        struct cs1300bmp *output = new struct cs1300bmp;
        int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);
        
        if ( ok ) {
            double sample = applyFilter(filter, input, output);
            sum += sample;
            samples++;
            cs1300bmp_writefile((char *) outputFilename.c_str(), output);
        }
    }
    fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);
    
}

struct Filter *
readFilter(string filename)
{
    ifstream input(filename.c_str());
    
    if ( ! input.bad() )
    {
        int size = 0;
        input >> size;
        Filter *filter = new Filter(size);
        int div;
        input >> div;
        filter -> setDivisor(div);
        for (int i=0; i < size; i++) {
            for (int j=0; j < size; j++) {
                int value;
                input >> value;
                filter -> set(i,j,value);
            }
        }
        return filter;
    }
    
    return NULL;
}

double applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{
    
    long long cycStart, cycStop;
    
    cycStart = rdtscll();
    
    output -> width = input -> width;
    output -> height = input -> height;
    
    int filtSize = filter -> getSize();         //Store loop invariants locally
    int filterDivisor = filter -> getDivisor(); //to reduce function calls
    int inputWidth = (input -> width) - 1;
    int inputHeight = (input -> height) - 1;
    
    
    int filterMatrix[filtSize][filtSize];
    
    for(int a = 0; a < filtSize; ++a)       //Store filter matrix to reduce
    {                                       //function calls in inner loops
        for(int b = 0; b < filtSize; ++b)
        {
            filterMatrix[b][a] = filter->get(a, b);
        }
    }
    
    
    
    for(int col = 1; col < inputWidth; ++col)
    {
        for(int row = 1; row < inputHeight; ++row)
        {
            
            //  Code between the comment lines executes for each pixel
            //------------------------------------------------------------------
            
            int val0 = 0;
            int val1 = 0;
            int val2 = 0;
            for (int j = 0; j < filtSize; ++j)
            {
                int tempCol = col + j -1;
                for (int i = 0; i < filtSize; ++i)
                {
                    int tempRow = row + i - 1;
                    int filterGet = filterMatrix[j][i];
                    //Unrolled plane loop for better pipelining
                    //Seperate variables, fewer jumps, less dependencies
                    val0 += input -> color[tempCol][tempRow][0] * filterGet;
                    val1 += input -> color[tempCol][tempRow][1] * filterGet;
                    val2 += input -> color[tempCol][tempRow][2] * filterGet;
                }
            }
            
            val0 /= filterDivisor;
            val1 /= filterDivisor;
            val2 /= filterDivisor;
            
            
            
            val0 = val0 < 0   ? 0   : val0; //I read that using these might be
            val1 = val1 < 0   ? 0   : val1; //slightly better than if statements
            val2 = val2 < 0   ? 0   : val2; //Not sure why. Less jumps or
                                            //Something? Maybe better dependency
            val0 = val0 > 255 ? 255 : val0;
            val1 = val1 > 255 ? 255 : val1;
            val2 = val2 > 255 ? 255 : val2;

            
            output -> color[col][row][0] = val0;
            output -> color[col][row][1] = val1;
            output -> color[col][row][2] = val2;
            
            //------------------------------------------------------------------
        }
    }
    
    cycStop = rdtscll();
    double diff = cycStop - cycStart;
    double diffPerPixel = diff / (output -> width * output -> height);
    fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
            diff, diff / (output -> width * output -> height));
    return diffPerPixel;
}
