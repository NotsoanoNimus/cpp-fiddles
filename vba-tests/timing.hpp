#ifndef _TIMING_H_
#define _TIMING_H_

#include <vector>
#include <tuple>
#include <chrono>
#include <string>
#include <sstream>
#include <stdlib.h>


class Timing
{
public:
    static void RecordTiming(uint64_t slot, uint64_t microseconds, std::string comment = "");
    static void RecordTiming(uint64_t slot,
                             std::chrono::high_resolution_clock::time_point start,
                             std::chrono::high_resolution_clock::time_point end,
                             std::string comment = "");

    static void DumpToCsv(std::string with_name, std::stringstream& to_stream);

    static uint64_t ConvertTimeToMicroseconds(std::chrono::high_resolution_clock::time_point start,
                                              std::chrono::high_resolution_clock::time_point end);

private:
    static std::vector<std::tuple<uint64_t, uint64_t, double, double, std::string>> timing_results;
};


#endif /* _TIMING_H_ */