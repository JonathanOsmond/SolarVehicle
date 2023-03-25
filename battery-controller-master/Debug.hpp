#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <mbed.h>

class Debug : public Serial {
    public:
        Debug();

        int debug(const char * function, uint32_t line, const char * level, const char * format, ...) __attribute__((format(printf, 4, 0)));

        void debug_can(const char * function, uint32_t line, const char * msg, const CANMessage & can);

        template<typename T>
            void debug_array(const char * function, uint32_t line, const char * msg,
                    const char * fmt, const T * array, uint32_t len) {
                lock();

                this->printf("[DEBUG - %s():%u] %s: [", function, line, msg);
                for(uint32_t i=0; i<len; ++i) {
                    this->printf(fmt, array[i]);
                    if(i != len-1)
                        this->printf(", ");
                }

                this->printf("]\n");

                unlock();
            }
};

extern Debug CONSOLE;


#ifdef NDEBUG

#define DEBUG(...)
#define INFO(...)
#define WARN(...)
#define ERROR(...)
#define DEBUG_CAN(...)
#define DEBUG_ARRAY(...)

#else

#define DEBUG(...) CONSOLE.debug(__FUNCTION__, __LINE__, "DEBUG", __VA_ARGS__)
#define INFO(...) CONSOLE.debug(__FUNCTION__, __LINE__, " INFO", __VA_ARGS__)
#define WARN(...) CONSOLE.debug(__FUNCTION__, __LINE__, " WARN", __VA_ARGS__)
#define ERROR(...) CONSOLE.debug(__FUNCTION__, __LINE__, "ERROR", __VA_ARGS__)
#define DEBUG_CAN(msg, can) CONSOLE.debug_can(__FUNCTION__, __LINE__, msg, can)
#define DEBUG_ARRAY(msg, fmt, array, len) CONSOLE.debug_array(__FUNCTION__, __LINE__, msg, fmt, array, len)

#endif

#endif
