#include <iostream>
#include <unistd.h>
//// #include <sys/mman.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <ctime>


#define LOG(x) std::cout << x << std::endl;

//RPM && WPM should work on sudo processes too

//Making this global to return from the init func, maybe change this
int fd_proc_mem;
unsigned long baseAddress;
const char * procName = "linux_64_client";

template <typename T>
void readMemory(unsigned long address, T* ret) {
    T* buf = (T*)malloc(sizeof(T));
    lseek(fd_proc_mem, address, SEEK_SET);
    read (fd_proc_mem, buf, sizeof(T));
    //ret=buf;
    *ret=*buf;
    free(buf);
}

template <typename T>
void writeMemory(unsigned long address, T val) {
    int size = sizeof(T);

    T* buf = (T*)malloc(size);
    *buf=val;
    lseek(fd_proc_mem, address, SEEK_SET);
    if (write (fd_proc_mem, buf , size) == -1) {
        std::cout << "Error while writing typesize " << sizeof(T) << " at address " << address << "\n";
    }
    free(buf);
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

unsigned long GetProcessBaseAddress(const char* mapFilesPath) {
        //Listing all the files in /proc/pid/map_files will show us some memory addresses
        //With the first one when sorted alphabetically being baseAddress-something
        //So we just need to call ls on the path, sort the output and get the first address after removing everything after -
        char* cmd = (char*)malloc(50);
        sprintf(cmd,"ls %s | sort",mapFilesPath);
        auto out = exec(cmd);
        //remove everything after -
        out = out.substr(0, out.find('-'));
        std::stringstream ss(out);
        delete[] cmd;
        void* result;
        ss >> result;
        return (unsigned long)result;
}

//Gets process id from process name using ps
int GetPid(const char* processName) {
    char* cmd = (char*)malloc(50);
    sprintf(cmd,"ps -A | grep \"%s\"",processName);
    auto out = exec(cmd);

    //if process isn't open
    if (out.length()==0) {
        delete[] cmd;
        return -1;
    }

	//im sure this is a memory leak but i won't fix it atm
    int i=0,j=0;
    for (;out[i]==' '; i++) {}
    for(;out[i]!=' ';j++,i++) {
        cmd[j]=out[i];
    }
    cmd[j]='\0';
    int pid = std::stoi(cmd);
    delete[] cmd;
    return pid;
}

//Inits the app
void init() {
    int pid = GetPid(procName);
    std::cout << "PID: " << pid << "\n";

    //we need to cast to char*
    char* proc_mem = (char*)malloc(50);

    //Make the char array "proc_mem" /proc/1234/mem , where 1234 is the pid
    sprintf(proc_mem, "/proc/%d/mem", pid);

    //Open that "file" that contains the process memory
    fd_proc_mem = open(proc_mem, O_RDWR);
    if (fd_proc_mem == -1) {
        std::cout<< "Could not open " << proc_mem << ", please run as root\n";
        exit(1);
    }

    sprintf(proc_mem, "/proc/%d/map_files", pid);

    baseAddress = GetProcessBaseAddress(proc_mem);
}

//REMEMBER THAT USUALLY OFFSETS ARE IN HEX NOT DEC
//Assault cube example
static unsigned long _hpOffset=0x100;
static unsigned long _armorOffset = 0x104;
static unsigned long _weapon2AmmoOffset = 0x140;
static unsigned long _weapon1AmmoOffset = 0x154;
static unsigned long _grenadeOffset = 0x158;
static unsigned long _playerOffset = 0x1A2518;

static bool _quit = false;
int main() {
    init();
    //get pointer bcs it points to another pointer
    readMemory(baseAddress+_playerOffset,&_playerOffset);
    //BA+O1 -> P1, P1->Entity
    int hp;
    readMemory(_playerOffset+_hpOffset,&hp);

    std::cout<<"HP before editing is "<<hp<<std::endl;

    while (true) {
        writeMemory(_playerOffset+_hpOffset,1337);
        writeMemory(_playerOffset+_weapon1AmmoOffset,1337);
        writeMemory(_playerOffset+_weapon2AmmoOffset,1337);
        writeMemory(_playerOffset+_armorOffset,1337);
        writeMemory(_playerOffset+_grenadeOffset,1337);
        nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);

        if (_quit == true) return 0;
    }
}
