#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <array>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

double readThroughput(std::string parseOutput) {
    std::istringstream output(parseOutput);
    std::string line;
    double result = 0;
    int numResults = 0;
    while (std::getline(output, line)) {
        int pos = 0;
        for (int i=0; i<5; i++) {
            pos = line.find('\t', pos);
            if (pos < 0) {
                std::cerr << "Command printed out a line with less than 5 fields in it:\n" << line << std::endl;
            }
            pos++;
        }
        result += std::stod(line.substr(pos));
        numResults++;
    }
    return result / numResults;
}

const double INTERLEAVED_ATTEMPTS = 7;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <new parse cmd> <reference parse cmd>" << std::endl;
        return 1;
    }
    std::vector<double> ref;
    std::vector<double> newcode;
    for (int attempt=0; attempt < INTERLEAVED_ATTEMPTS; attempt++) {
        std::cout << "Attempt #" << (attempt+1) << " of " << INTERLEAVED_ATTEMPTS << std::endl;

        // Read new throughput
        double newThroughput = readThroughput(exec(argv[1]));
        std::cout << "New throughput: " << newThroughput << std::endl;
        newcode.push_back(newThroughput);

        // Read reference throughput
        double referenceThroughput = readThroughput(exec(argv[2]));
        std::cout << "Ref throughput: " << referenceThroughput << std::endl;
        ref.push_back(referenceThroughput);
    }
    // we check if the maximum of newcode is lower than minimum of ref, if so we have a problem so fail!
    double worseref = *std::min_element(ref.begin(), ref.end());
    double bestnewcode =  *std::max_element(newcode.begin(), newcode.end());
    double bestref = *std::max_element(ref.begin(), ref.end());
    double worsenewcode =  *std::min_element(newcode.begin(), newcode.end());
    std::cout << "The new code has a throughput in       " << worsenewcode << " -- " << bestnewcode << std::endl;
    std::cout << "The reference code has a throughput in " << worseref << " -- " << bestref << std::endl;
    if(bestnewcode < worseref) {
      std::cerr << "You probably have a performance degradation." << std::endl;
      return EXIT_FAILURE;
    }
    if(bestnewcode < worseref) {
      std::cout << "You probably have a performance gain." << std::endl;
      return EXIT_SUCCESS;
    }
    std::cout << "There is no obvious performance difference. A manual check might be needed." << std::endl;
    return EXIT_SUCCESS;
}
