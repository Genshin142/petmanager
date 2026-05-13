cmake_minimum_required(VERSION 3.16)

# 解决高版本 CMake 对旧策略的兼容性报错
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(PetServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 自动工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. 自动化获取 glog 库
include(FetchContent)
FetchContent_Declare(
  glog
  GIT_REPOSITORY https://github.com/google/glog.git
  GIT_TAG v0.7.0
)

# 针对 Windows MinGW 环境优化配置，关闭不兼容的特性
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(WITH_GTEST OFF CACHE BOOL "" FORCE)
set(WITH_SYMBOLIZE OFF CACHE BOOL "" FORCE)
set(WITH_UNWIND OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) 

FetchContent_MakeAvailable(glog)

# 2. 自动查找 Qt 6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

# 2. 包含头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/database)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/network)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/logic)

# 3. 定义源文件
set(SOURCES
    main.cpp
    database/connectionpool.cpp
    network/client_handler.cpp
    network/server_core.cpp
    logic/user_controller.cpp
    logic/pet_controller.cpp
    logic/member_controller.cpp
    logic/product_controller.cpp
    logic/staff_controller.cpp
    logic/service_controller.cpp
    logic/schedule_controller.cpp
    logic/appointment_controller.cpp
    logic/order_controller.cpp
    logic/finance_controller.cpp
    logic/report_controller.cpp
)

# 4. 生成可执行程序
add_executable(PetServer ${SOURCES})

# 5. 链接库
target_link_libraries(PetServer PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Sql
    glog::glog
)

# 6. 控制台输出配置
if(WIN32)
    set_target_properties(PetServer PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
