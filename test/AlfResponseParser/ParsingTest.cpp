#include "AlfResponseParser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
#include <cstdlib>  // For std::atoi
#include <chrono>   // For time measurement

int main(int argc, const char** argv)
{
    //Determine the number of lines based on command-line argument
    int num_lines = 2; // Default number of lines
    if (argc > 1)
    {
        num_lines = std::atoi(argv[1]);
        if (num_lines < 0) num_lines = 0; // Ensure non-negative number of lines
    }

    // Generate the 'data' string randomly
    std::string data = "success";

    // Set up random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist_p(0,1);
    std::uniform_int_distribution<uint16_t> dist_prefix(0, 0xFFF); // 3 hex digits
    std::uniform_int_distribution<uint32_t> dist_32bit(0, 0xFFFFFFFF); // 8 hex digits

    for (int i = 0; i < num_lines; ++i)
    {
        std::stringstream ss;
        ss <<"\n";
        if(dist_p(gen) < 0.5)
        {
            ss << "0";
        } 
        else
        {
        ss << "0x"
           << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << dist_prefix(gen)
           << std::setw(8) << dist_32bit(gen)
           << std::setw(8) << dist_32bit(gen);
        }
        data += ss.str();
    }

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

    std::string parsed = parsedStream.str();
    if(parsed.size() != data.size())
    {
        std::cout<< "Expected: " << data.size() << " chars. Get: " << parsed.size() << std::endl;
    }
    for(int i = 0; i < parsed.size(); i++)
    {
        if(parsed[i] != data[i])
        {
            std::cout << "Parsing went wrong at position " << i << std::endl;
            return -1;
            break;
        }
    }
    std::cout << std::endl << "Parsing was successful" << std::endl;

    std::string to_parse = "success\n0\n0x00000000000ffffffff\n0x00000000000afffffff";
    AlfResponseParser testpars(to_parse);
    for(auto line: testpars)
    {
        std::cout << line.length << std::endl;
    }

    return 0;
}
