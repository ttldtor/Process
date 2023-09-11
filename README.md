# Process
Cross-platform library for obtaining metrics of the current process

Status:
* [x] Windows
* [x] Linux
* [ ] MacOS
* [ ] FreeBSD
* [ ] AIX
* [ ] ...

Example:

```cpp
#include <process/process.hpp>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::literals;
using namespace ttldtor::process;

int main() {
    auto now = [] {
        return duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    };
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;

    auto t1 = now();

    while (now() - t1 <= 5) {
        static std::size_t i = 0;
        (void)(i++);
    }

    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::vector<std::byte> vb(10 * 1024 * 1024);
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    vb.resize(1);
    vb.shrink_to_fit();
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
}
```

```text
Physical memory usage: 3676KB
Total CPU time: 15ms
Total CPU time: 5687ms
Physical memory usage: 3720KB
Physical memory usage: 13964KB
Physical memory usage: 3720KB
```