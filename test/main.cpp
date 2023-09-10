#include <process/process.hpp>

#include <iostream>
#include <thread>
#include <chrono>

using namespace std::literals;
using namespace ttldtor::process;

int main() {
    auto now = [] { return duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); };

    std::cout << Process::getTotalProcessorTime() << std::endl;

    auto t1 = now();

    while (now() - t1 < 5) {
        static std::size_t i = 0;
        (void)(i++);
        //(void)randomInt();
    }

    std::cout << std::endl;

    std::cout << Process::getTotalProcessorTime() << std::endl;
}
