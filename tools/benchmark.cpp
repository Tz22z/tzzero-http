#include <iostream>
#include <string>
#include <getopt.h>

// 简单的基准测试工具占位符
// 建议使用专业工具如 wrk, ab, hey 进行真实的性能测试

void print_usage(const char* program) {
    std::cout << "TZZero HTTP Benchmark Tool\n\n"
              << "Usage: " << program << " [OPTIONS]\n"
              << "Options:\n"
              << "  -u, --url URL           Target URL (default: http://localhost:3000/)\n"
              << "  -c, --connections NUM   Number of connections (default: 100)\n"
              << "  -d, --duration SEC      Test duration in seconds (default: 10)\n"
              << "  -t, --threads NUM       Number of threads (default: 4)\n"
              << "  -h, --help              Show this help message\n\n"
              << "Note: This is a placeholder tool. For serious benchmarking, use:\n"
              << "  • wrk: https://github.com/wg/wrk\n"
              << "  • ab (Apache Bench): comes with Apache\n"
              << "  • hey: https://github.com/rakyll/hey\n\n"
              << "Example with wrk:\n"
              << "  wrk -t4 -c100 -d10s http://localhost:3000/\n\n"
              << "Example with ab:\n"
              << "  ab -n 10000 -c 100 http://localhost:3000/\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string url = "http://localhost:3000/";
    int connections = 100;
    int duration = 10;
    int threads = 4;

    struct option long_options[] = {
        {"url", required_argument, 0, 'u'},
        {"connections", required_argument, 0, 'c'},
        {"duration", required_argument, 0, 'd'},
        {"threads", required_argument, 0, 't'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "u:c:d:t:h", long_options, nullptr)) != -1) {
        switch (c) {
            case 'u':
                url = optarg;
                break;
            case 'c':
                connections = std::stoi(optarg);
                break;
            case 'd':
                duration = std::stoi(optarg);
                break;
            case 't':
                threads = std::stoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    std::cout << "\n=== TZZero HTTP Benchmark Configuration ===\n";
    std::cout << "Target URL:    " << url << "\n";
    std::cout << "Connections:   " << connections << "\n";
    std::cout << "Duration:      " << duration << " seconds\n";
    std::cout << "Threads:       " << threads << "\n";
    std::cout << "===========================================\n\n";

    std::cout << "⚠️  This is a configuration validator only.\n";
    std::cout << "📊 To run actual benchmarks, use professional tools:\n\n";

    std::cout << "Recommended command with wrk:\n";
    std::cout << "  wrk -t" << threads << " -c" << connections
              << " -d" << duration << "s " << url << "\n\n";

    std::cout << "Recommended command with ab:\n";
    std::cout << "  ab -n " << (connections * duration * 100)
              << " -c " << connections << " " << url << "\n\n";

    std::cout << "Recommended command with hey:\n";
    std::cout << "  hey -z " << duration << "s -c " << connections
              << " " << url << "\n\n";

    return 0;
}