cmake_minimum_required(VERSION 3.16)

project(PetServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 自动工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. 自动查找 Qt 6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

# 2. 包含头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/database)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/network)

# 3. 定义源文件
set(SOURCES
    main.cpp
    database/connectionpool.cpp
    network/client_handler.cpp
    network/server_core.cpp
)

# 4. 生成可执行程序
add_executable(PetServer ${SOURCES})

# 5. 链接 Qt 库
target_link_libraries(PetServer PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Sql
)

# 6. 控制台输出配置
if(WIN32)
    set_target_properties(PetServer PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
