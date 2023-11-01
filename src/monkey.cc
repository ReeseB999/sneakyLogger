#include <future>
#include <string>
#include <vector>

#include "monkey.hh"

int main(int argc, char** argv) {
    // Process command-line arguments
    bool enableDebug = false;
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            enableDebug = true;
            break;
        }
    }

    // Discover available keyboards
    std::vector<Keyboard> keyboards = findKeyboards();
    int totalKeyboards = keyboards.size();

    // Start capturing keystrokes concurrently
    std::vector<std::future<void>> captureTasks;
    for (auto& kbd : keyboards) {
        captureTasks.push_back(std::async(std::launch::async, [&kbd]() {
            kbd.capture();
        }));
    }

    // Wait for all capture tasks to complete
    for (const auto& task : captureTasks) {
        task.wait();
    }

    return 0;
}
