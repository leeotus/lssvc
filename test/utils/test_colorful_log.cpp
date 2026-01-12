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
const std::string RESET = "\033[0m";  // 重置颜色

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

    // 输出线程ID
    stream_ << "[Thread:" << std::this_thread::get_id() << "] ";

    // 输出带颜色的日志级别
    stream_ << level_colors[level] << log_string[level] << RESET;

    // 输出文件名和行号
    stream_ << " [" << file_name << ":" << line << "]" << std::endl;
}

// 使用示例
int main() {
    colored_log(0, "main.cpp", 10);  // ERROR - 红色
    colored_log(1, "main.cpp", 11);  // WARNING - 黄色
    colored_log(2, "main.cpp", 12);  // INFO - 白色
    colored_log(3, "main.cpp", 13);  // DEBUG - 绿色
    colored_log(4, "main.cpp", 14);  // TRACE - 青色
    return 0;
}
