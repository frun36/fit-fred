#include "AlfResponseParser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
#include <cstdlib>  // For std::atoi
#include <chrono>   // For time measurement

int main(int argc, const char** argv)
{
    // Determine the number of lines based on command-line argument
    int num_lines = 2; // Default number of lines
    if (argc > 1)
    {
        num_lines = std::atoi(argv[1]);
        if (num_lines < 0) num_lines = 0; // Ensure non-negative number of lines
    }

    // Generate the 'data' string randomly
    std::string data = "success\n";

    // Set up random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dist_prefix(0, 0xFFF); // 3 hex digits
    std::uniform_int_distribution<uint32_t> dist_32bit(0, 0xFFFFFFFF); // 8 hex digits

    for (int i = 0; i < num_lines; ++i)
    {
        std::stringstream ss;
        ss << "0x"
           << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << dist_prefix(gen)
           << std::setw(8) << dist_32bit(gen)
           << std::setw(8) << dist_32bit(gen)
           << '\n';
        data += ss.str();
    }

    data += '0';

    // Now proceed with parsing and comparison
    AlfResponseParser parser(data.c_str());

    std::stringstream parsedStream;
    parsedStream << (parser.isSuccess() ? "success"  : "failure");

    // Start measuring time
    auto start_time = std::chrono::high_resolution_clock::now();
    int lines_processed = 0;
    // Loop that we want to measure in microseconds
    for (auto line : parser)
    {
        lines_processed++;
    }

    // Stop measuring time
    auto end_time = std::chrono::high_resolution_clock::now();
    // Calculate the duration in microseconds
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Time taken to process the parser loop: " << duration << " microseconds" << std::endl;
    if (lines_processed > 0) {
        double avg_time_per_line = static_cast<double>(duration) / lines_processed;
        std::cout << "Average time per line: " << avg_time_per_line << " microseconds" << std::endl;
    } else {
        std::cout << "No lines were processed." << std::endl;
    }

    for (auto line : parser)
    {
        parsedStream.put('\n');

        if (line.type == AlfResponseParser::Line::Type::ResponseToWrite) {
            parsedStream << '0';
        } else {
            parsedStream << "0x"
                << std::hex << std::setw(3) << std::setfill('0') << std::uppercase << line.frame.prefix
                << std::setw(8) << line.frame.address
                << std::setw(8) << line.frame.data;
        }
    }

    

    int64_t pos = 0;
    char c;

    while (!parsedStream.eof()) {
        if ((c = parsedStream.get()) == EOF) {
            break;
        }
        
        //std::cout << c;

        if (c != data[pos++]) {
            std::cout << "Parsing went wrong at position " << pos << std::endl;
            break;
        }
    }

    if (parsedStream.eof()) {
        std::cout << std::endl << "Parsing was successful" << std::endl;
        return 0;
    }

    return -1;
}
