#include "Debug.hpp"

Debug CONSOLE;

Debug::Debug() : Serial(USBTX, USBRX) {
    baud(460800);
    lock();
    this->printf("\n\n Build: " __DATE__ __TIME__ "\n");
    unlock();
}

int Debug::debug(const char * function, uint32_t line, const char * level, const char * format, ...) {
    lock();
    int r = this->printf("[%s - %s():%u] ", level, function, line);
    
    std::va_list arg;
    va_start(arg, format);
    r += this->vprintf(format, arg);
    va_end(arg);

    r += this->printf("\n");
    unlock();
    return r;
}

void Debug::debug_can(const char * function, uint32_t line, const char * msg, const CANMessage & can) {
    lock();

    this->printf("[DEBUG - %s():%u] %s: 0x%X - ", function, line, msg, can.id);
    for(int i=0; i<can.len; ++i)
        this->printf("%02hhX ", can.data[i]);

    this->printf("\n");

    unlock();
}
