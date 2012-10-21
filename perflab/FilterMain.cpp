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
    
    if ( ! input.bad() ) {
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
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{
    
    long long cycStart, cycStop;
    
    cycStart = rdtscll();
    
    output -> width = input -> width;
    output -> height = input -> height;
    
    int filtSize = filter -> getSize();     //reduce func calls in inner loops
//    int val0, val1, val2 = 0;
//    int filterGet = 0;
//    int tempRow = 0;
//    int tempCol = 0;
    int filterDivisor = filter -> getDivisor();
    int tempVal0, tempVal1, tempVal2 = 0;
    int inputWidth = (input -> width) - 1;
    int inputHeight = (input -> height) - 1;
    
    
    
    
    for(int col = 1; col < inputWidth; ++col)
    {
        for(int row = 1; row < inputHeight; ++row)
        {
            
            //  Code between the comment lines executes for each pixel
            //------------------------------------------------------------------
            
            /*
            for(int plane = 0; plane < 3; plane++)  //1 iteration for each color
            {
                
                int value = 0;
                for (int j = 0; j < filtSize; j++)
                {
                    for (int i = 0; i < filtSize; i++)
                    {
                        value = value +  input -> color[plane][row + i - 1][col + j - 1]* filter -> get(i, j);
                    }
                }
                
                
                value = value / filter -> getDivisor();
                if ( value  < 0 ) { value = 0; }
                if ( value  > 255 ) { value = 255; }
                output -> color[plane][row][col] = value;
                
            }
            */
            
            int val0, val1, val2 = 0;
            for (int j = 0; j < filtSize; ++j)  //combine these two loops
            {                                   //into 1 loop that uses 1
                int tempCol = col + j -1;           //var and modulus or some shit
                for (int i = 0; i < filtSize; ++i)
                {
                    int tempRow = row + i - 1;
                    int filterGet = filter -> get(i,j);
                    
                    /*
                    tempVal0 = input -> color[0][tempRow][tempCol] * filterGet;
                    tempVal1 = input -> color[1][tempRow][tempCol] * filterGet;
                    tempVal2 = input -> color[2][tempRow][tempCol] * filterGet;
                    
                    val0 += tempVal0;
                    val1 += tempVal1;
                    val2 += tempVal2;
                    */
                     
                    
                    val0 += input -> color[0][tempRow][tempCol] * filterGet;
                    val1 += input -> color[1][tempRow][tempCol] * filterGet;
                    val2 += input -> color[2][tempRow][tempCol] * filterGet;
                     

                }
            }
            
            
            val0 = val0 / filterDivisor;
            val1 = val1 / filterDivisor;
            val2 = val2 / filterDivisor;
            
            if ( val0 < 0 ) { val0 = 0; }
            if ( val0  > 255 ) { val0 = 255; }
            
            if ( val1 < 0 ) { val1 = 0; }
            if ( val1  > 255 ) { val1 = 255; }
            
            if ( val2 < 0 ) { val2 = 0; }
            if ( val2  > 255 ) { val2 = 255; }
            
            output -> color[0][row][col] = val0;
            output -> color[1][row][col] = val1;
            output -> color[2][row][col] = val2;
            
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
