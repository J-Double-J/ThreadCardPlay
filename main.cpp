#include <iostream>
#include <getopt.h>
#include <vector>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <ctime>
#include <algorithm>

using namespace std;

void handleOptions(int, char **, uint32_t&, uint64_t&, uint32_t&);
void initializeNumbers(vector<uint32_t>&, uint64_t&, uint32_t&);
void launchThreads(vector<uint32_t>&, uint32_t&, uint32_t&);
void checkSpitPile(vector<uint32_t>*, uint32_t, uint32_t, uint16_t);

mutex mtx;

uint32_t numToSpit = 0;

int main(int argc, char **argv) {
    uint32_t size = 1 << 20;
    uint64_t seed = time(NULL);
    uint32_t nthreads = 4;
    vector<uint32_t>  numbers;

    handleOptions(argc, argv, size, seed, nthreads);
    initializeNumbers(numbers, seed, size);
    launchThreads(numbers, nthreads, size);

    return 0;
}

//getopt code that handles options given
void handleOptions(int argc, char ** argv, uint32_t& size, uint64_t& seed, uint32_t& nthreads) {
    int c;
    while ((c = getopt(argc, argv, "z:s:t:h")) != -1) {
        switch (c) {
            case 'z':
                size = 1 << atoi(optarg);
                if (size < 256)
                    size = 1 << 8;              //bit bash to 256
                break;

            case 's':
                seed = atoi(optarg);
                break;

            case 't':
                nthreads = atoi(optarg);
                if (nthreads < 4)
                    nthreads = 4;
                else if (nthreads > 30)
                    nthreads = 30;
                break;

            case 'h':
                cerr << "Usage:\n";
                cerr << "-z N    Sets value of size to 1 << N - default: 1 << 20" << endl;
                cerr << "-s N    Sets value of seed to N - default: current time" << endl;
                cerr << "-t N   Sets number of threads to copy to N - default: 4; max: 30" << endl;
                exit(1);
        }
    }
}

//Get a vector of numbers equal to size. Shuffle the numbers depending on the seed given
void initializeNumbers(vector<uint32_t>& numbers, uint64_t& seed, uint32_t& size) {
    srand((uint32_t)seed);

    //generate numbers
    for (uint32_t i = 0; i < size; i++)
        numbers.push_back(i);

    //shuffle using seed
    for (uint32_t i = 0; i < size; i++) {
        uint32_t shuffleTarget = rand() % (size-1);
        uint32_t shuffleNum = numbers.at(shuffleTarget);
        numbers.at(shuffleTarget) = numbers.at(i);
        numbers.at(i) = shuffleNum;
    }
}

//calculates how many cards are in each hand and with the given number of vectors
//the beginning threads get the smallest hands. Joins threads once threads are done
void launchThreads(vector<uint32_t>& numbers, uint32_t& nthreads, uint32_t& size) {
    
    vector<thread> threads;
    uint32_t vectorSizes = size/nthreads;
    uint32_t remainder = size - (vectorSizes * nthreads);   //Will be less then nthreads
    uint16_t regularVectors = nthreads - remainder;         //Num of minimum size vectors
    uint8_t offset = 0;                                     //As extra cards are added to specific hands, don't redeal same cards
    

    //deal hands
    for (uint16_t i = 0; i < nthreads; i++) {
        //If vector doesnt need to hold extra. Example hold from position 0 to position 4 if first thread, and vector size is 4
        if (i < regularVectors)
            threads.push_back(thread(std::bind(&checkSpitPile, &numbers, i*vectorSizes, (i*vectorSizes)+vectorSizes, i)));
        else {
        threads.push_back(thread(std::bind(&checkSpitPile, &numbers, i*vectorSizes+offset, (i*vectorSizes)+vectorSizes+offset+1, i)));     //Hold one extra
            offset++;
        }
    }

    //join threads
    for (uint32_t i = 0; i < nthreads; i++) {
        threads.at(i).join();
    }
}

//Sort assigned hand and then checks the spit file. If number is found, locks mutex prints
//statement then increment pile and continues to look at next lowest card.
void checkSpitPile(vector<uint32_t>* numbers, uint32_t begin, uint32_t end, uint16_t id) {
    sort(numbers->begin()+begin, numbers->begin()+end);
    
    uint32_t pos = begin;

    while(pos != end) {
        cout << "";
        if (numbers->at(pos) == numToSpit) {
            mtx.lock();
            cout << "Thread: " << id << " hit on: " << numbers->at(pos) << endl;
            numToSpit++;
            pos++;
            mtx.unlock();
        }
    }
}
