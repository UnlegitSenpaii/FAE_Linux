cmake_minimum_required(VERSION 3.25)
project(FactorioAchievementEnabler)

set(CMAKE_INCLUDE_CURRENT_DIR On)

add_executable(fae
    fae/filehelper.cpp
    fae/filehelper.hpp
    fae/logging.cpp
    fae/logging.hpp
    fae/patching.cpp
    fae/patching.hpp
    fae/main.cpp)

target_compile_features(fae
    PRIVATE
        cxx_std_20)
