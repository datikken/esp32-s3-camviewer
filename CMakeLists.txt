cmake_minimum_required(VERSION 3.10)
project(esp32s3-cam-viewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# --- OpenCV ---
find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc videoio)

# --- libcurl (опционально, для расширенного режима) ---
find_package(CURL QUIET)

add_executable(esp32s3-cam-viewer src/main.cpp
        src/main.cpp)

target_include_directories(esp32s3-cam-viewer PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    src
)

target_link_libraries(esp32s3-cam-viewer PRIVATE
    ${OpenCV_LIBS}
    pthread
)

if(CURL_FOUND)
    target_compile_definitions(esp32s3-cam-viewer PRIVATE HAVE_CURL=1)
    target_link_libraries(esp32s3-cam-viewer PRIVATE ${CURL_LIBRARIES})
endif()

# --- Установка ---
install(TARGETS esp32s3-cam-viewer RUNTIME DESTINATION bin)
