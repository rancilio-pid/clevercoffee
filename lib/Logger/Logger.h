#pragma once

#include <WiFiManager.h>

#include <stdint.h>

class Logger {
    public:
        enum class Level : int {
            TRACE = 0,
            DEBUG = 1,
            INFO = 2,
            WARNING = 3,
            ERROR = 4,
            FATAL = 5,
            SILENT = 6,
        };

        /**
         * @brief Return singleton instance of the logger
         * @return Logger instance
         */
        static Logger& getInstance() {
            return getInstanceImpl();
        }

        /**
         * @brief Initialize the singleton logger instance
         *
         * @param port Port on which the logger should listen for client connections
         */
        static void init(const uint16_t port);

        Logger(Logger const&) = delete;
        void operator=(Logger const&) = delete;

        /**
         * @brief Start the logger
         * @details This method should be called in the setup() function of the main program.
         * @return Boolean indicating the success of the operation
         */
        static bool begin();

        /**
         * @brief Updating of the logger
         * @details This method handles all incoming connections and registers new clients. It therefore needs to be called
         * in every iteration of the main program loop() function.
         *
         * @return Boolean indicating the success of the operation
         */
        static bool update();

        /**
         * @brief Get the port this logger communicating over
         *
         * @return Used port
         */
        static uint16_t getPort();

        /**
         * @brief Send a log message either via serial or serial-over-wifi
         * @details This method takes a log level, source location infos and a string, assembles a log message and sends it
         *          either via serial bus or over wifi to connected clients.

         * @param level String setting the log level
         * @param file The file name of the file containing the log message
         * @param function The function containing the log message
         * @param line The line number of the log message
         * @param logmsg Log message to be sent as payload
         */
        void log(const Level level, const String& file, const __FlashStringHelper* function, uint32_t line, const char* logmsg);

        /**
         * @brief Send a log message either via serial or serial-over-wifi
         * @details This method takes a log level, source location infos and a format string, followed by parameters for the
         *          format string. It assembles a log message and sends it over serial or wifi to connected clients.
         *
         * @param level String setting the log level
         * @param file The file name of the file containing the log message
         * @param function The function containing the log message
         * @param line The line number of the log message
         * @param format Format string akin to printf
         * @param ... Parameter list
         */
        void logf(const Level level, const String& file, const __FlashStringHelper* function, uint32_t line, const char* format, ...);

        static void setLevel(Level level) {
            getInstance().level_ = level;
        }
        static Level getCurrentLevel() {
            return getInstance().level_;
        }

    private:
        static Logger& getInstanceImpl(const uint16_t port = 23);

        /**
         * @brief Constructor for a logger
         * @details This class allow to publish log messages via serial and wifi connections
         *
         * @param port Port on which the logger should listen for client connections
         */
        Logger(const uint16_t port = 23);

        void current_time(char* timestamp);

        String get_level_identifier(Level lvl);

        // Logging level
        Level level_{Level::INFO};

        // Port of this logger
        uint16_t port_;

        // Server and client
        WiFiClient client_;
        WiFiServer server_;
};

#ifndef __FILE_NAME__
/**
 *  @brief Base name of the file without the directory
 */
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/**
 * @brief Execute a block only if the reporting level is high enough
 * @param level The minimum log level
 */
#define IFLOG(level) if (Logger::Level::level >= Logger::getCurrentLevel())

#define LOG(level, ...)                                                                                                                                                                                                         \
    if (Logger::Level::level >= Logger::getCurrentLevel()) Logger::getInstance().log(Logger::Level::level, __FILE_NAME__, FPSTR(__func__), __LINE__, __VA_ARGS__)

#define LOGF(level, ...)                                                                                                                                                                                                        \
    if (Logger::Level::level >= Logger::getCurrentLevel()) Logger::getInstance().logf(Logger::Level::level, __FILE_NAME__, FPSTR(__func__), __LINE__, __VA_ARGS__)

// Some comment on __func__: this resides on flash storage, so we need to access it using FPSTR(). Copying it to string is
// cumbersome, so passing through the pointer and creating the final object directly from the __FlashStringHelper pointer.
