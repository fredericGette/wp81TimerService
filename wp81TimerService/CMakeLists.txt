cmake_minimum_required(VERSION 3.20)
project(wp81TimerService)

# Collect all .cpp files in src/ automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(wp81TimerService ${SOURCES})

target_compile_definitions(wp81TimerService PRIVATE
    _WIN32_WINNT=0x0603
    WINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP
)

target_include_directories(wp81TimerService PRIVATE
    "C:/Program Files (x86)/Windows Phone Kits/8.1/Include"
    "C:/Program Files (x86)/Windows Phone Kits/8.1/Include/abi"
    "C:/Program Files (x86)/Windows Phone Kits/8.1/Include/mincore"
    "C:/Program Files (x86)/Windows Phone Kits/8.1/Include/minwin"
    "C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/WPSDK/include"
)

target_compile_options(wp81TimerService PRIVATE
    /clang:-fno-sized-deallocation

    # Mark SDK paths as external (system) headers - warnings suppressed there
    "SHELL:/external:I \"C:/Program Files (x86)/Windows Phone Kits/8.1/Include\""
    "SHELL:/external:I \"C:/Program Files (x86)/Windows Phone Kits/8.1/Include/abi\""
    "SHELL:/external:I \"C:/Program Files (x86)/Windows Phone Kits/8.1/Include/mincore\""
    "SHELL:/external:I \"C:/Program Files (x86)/Windows Phone Kits/8.1/Include/minwin\""
    "SHELL:/external:I \"C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/WPSDK/include\""
    /external:W0
)

# Disable CMake's automatic default Windows libraries
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

# Explicitly link only what you need from your ARM SDK
target_link_directories(wp81TimerService PRIVATE
    "C:/Program Files (x86)/Windows Phone Kits/8.1/lib/ARM"
    "C:/Program Files (x86)/Windows Phone Kits/8.1/lib/winv6.3/um/arm"
    "C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/WPSDK/lib/arm"
)

target_link_libraries(wp81TimerService PRIVATE
    mincore.lib
)