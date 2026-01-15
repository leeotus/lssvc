#include <iostream>
#include <string>
#include <thread>

const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string RESET = "\033[0m";  // reset color

const std::string level_colors[] = {
    RED,     // ERROR
    YELLOW,  // WARNING
    WHITE,   // INFO
    GREEN,   // DEBUG
    CYAN     // TRACE
};

// 日志级别字符串
const std::string log_string[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
    "TRACE"
};

void colored_log(int level, const std::string& file_name, int line) {
    std::ostream& stream_ = std::cout;

    stream_ << "[Thread:" << std::this_thread::get_id() << "] ";
    stream_ << level_colors[level] << log_string[level] << RESET;
    stream_ << " [" << file_name << ":" << line << "]" << std::endl;
}

// example
int main() {
    colored_log(0, "main.cpp", 10);  // ERROR - red
    colored_log(1, "main.cpp", 11);  // WARNING - yellow
    colored_log(2, "main.cpp", 12);  // INFO - white
    colored_log(3, "main.cpp", 13);  // DEBUG - green
    colored_log(4, "main.cpp", 14);  // TRACE - cyan
    return 0;
}
