# Process
Cross-platform library for obtaining metrics of the current process

Status:
* [x] Windows
* [x] Linux
* [ ] MacOS
* [ ] FreeBSD
* [ ] AIX
* [ ] ...

### Example:

CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.20)

project(Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(Process GIT_REPOSITORY "https://github.com/ttldtor/Process.git" GIT_TAG default)
FetchContent_MakeAvailable(Process)

add_executable(${PROJECT_NAME}
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE process::process)

```

main.cpp
```cpp
#include <process/process.hpp>

#include <chrono>
#include <iostream>
#include <vector>

using namespace std::literals;
using namespace ttldtor::process;

int main() {
    auto now = [] {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;

    auto t1 = now();

    while (now() - t1 <= 5) {
        static std::size_t i = 0;
        (void)(i++);
    }

    std::cout << "Total CPU time (+5s): " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::vector<std::uint8_t> vb(10 * 1024 * 1024);
    std::cout << "Physical memory usage (+10MB): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    vb.resize(1);
    vb.shrink_to_fit();
    std::cout << "Physical memory usage (-10MB): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
}
```

```text
Physical memory usage: 4168KB
Total CPU time: 0ms
Total CPU time (+5s): 5609ms
Physical memory usage: 4212KB
Physical memory usage (+10MB): 14460KB
Physical memory usage (-10MB): 4216KB
```