#include "timing.hpp"


std::vector<std::tuple<uint64_t, uint64_t, double, double, std::string>> Timing::timing_results{};

uint64_t Timing::ConvertTimeToMicroseconds(std::chrono::high_resolution_clock::time_point start,
                                           std::chrono::high_resolution_clock::time_point end)
{
    std::chrono::duration<double> duration = end - start;
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}


void Timing::RecordTiming(uint64_t slot,
                          uint64_t microseconds,
                          std::string comment)
{
    timing_results.push_back(
        std::make_tuple(slot,
                        microseconds,
                        (double)microseconds / 1000.0,
                        (double)microseconds / 1000.0 / 1000.0,
                        comment)
    );
}


void Timing::RecordTiming(uint64_t slot,
                          std::chrono::high_resolution_clock::time_point start,
                          std::chrono::high_resolution_clock::time_point end,
                          std::string comment)
{
    return Timing::RecordTiming(slot, Timing::ConvertTimeToMicroseconds(start, end), comment);
}


void Timing::DumpToCsv(std::string with_name, std::stringstream& to_stream)
{
    for (int i = 0; i < (int)timing_results.size(); ++i) {
        to_stream << with_name << ","
            << std::get<0>(timing_results[i]) << ","
            << std::get<1>(timing_results[i]) << ","
            << std::get<2>(timing_results[i]) << ","
            << std::get<3>(timing_results[i]) << ","
            << std::get<4>(timing_results[i])
            << std::endl;
    }

    timing_results.clear();
}