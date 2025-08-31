cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(CrystalServer LANGUAGES CXX)

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Główne pliki źródłowe
add_executable(${PROJECT_NAME} main.cpp)

# Dodanie ikony dla Windows w sposób bardziej przejrzysty
if(MSVC)
    target_sources(${PROJECT_NAME} PRIVATE ../cmake/crystalserver.rc)
endif()

# Łączenie z biblioteką
target_link_libraries(${PROJECT_NAME} PRIVATE CrystalServer_lib)

# Ustawienie opcji kompilatora dla wszystkich platform i typów budowania
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Opcje specyficzne dla Debug
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g>
)

# Opcje specyficzne dla Release
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
)
