#include <process/process.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std::literals;
using namespace ttldtor::process;

int main() {
    auto now = [] { return duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); };
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime() << std::endl;

    auto t1 = now();

    while (now() - t1 <= 5) {
        static std::size_t i = 0;
        (void)(i++);
    }

    std::cout << "Total CPU time: " <<  Process::getTotalProcessorTime() << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::vector<std::byte> vb(10000000);
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    vb.resize(1);
    vb.shrink_to_fit();
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
}
