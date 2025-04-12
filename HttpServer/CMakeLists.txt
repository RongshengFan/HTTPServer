cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(HttpServer VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenSSL
find_package(OpenSSL REQUIRED)

# 查找 muduo 库
find_library(MUDUO_BASE muduo_base REQUIRED)
find_library(MUDUO_NET muduo_net REQUIRED)

# 查找 MySQL Connector/C++
find_library(MYSQLCPPCONN mysqlcppconn REQUIRED)

# 查找源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"        # 包含 src 目录下的所有 .cpp 文件
    "include/*.cpp"    # 如果有 .cpp 文件在 include 目录下
    "examples/*.cpp"   # 如果有 .cpp 文件在 examples 目录下
)

# 包含头文件目录
include_directories(
    include
    ${OPENSSL_INCLUDE_DIR}  # OpenSSL 头文件目录
)

# 添加可执行文件
add_executable(HttpServer ${SOURCES})

# 链接库
target_link_libraries(HttpServer 
    ${MUDUO_BASE} 
    ${MUDUO_NET} 
    ${OPENSSL_LIBRARIES}  # OpenSSL 库
    ${MYSQLCPPCONN}      # MySQL Connector/C++
    pthread
    ssl                  # 显式链接 SSL
    crypto              # 显式链接 crypto
)

# 如果使用了其他库（如 pthread），可以在这里链接
# find_package(Threads REQUIRED)
# target_link_libraries(HttpServer PRIVATE Threads::Threads)

# 如果有测试代码，可以添加测试目标
# enable_testing()
# add_subdirectory(tests)