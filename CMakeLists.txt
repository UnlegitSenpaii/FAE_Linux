cmake_minimum_required(VERSION 3.25)
project(FactorioAchievementEnabler)

add_executable(fae
    src/helpers/filehelper.cpp
    src/helpers/filehelper.hpp
    src/helpers/logging.cpp
    src/helpers/logging.hpp
    src/helpers/patching.cpp
    src/helpers/patching.hpp
    src/main.cpp)

target_compile_features(fae
    PRIVATE
        cxx_std_20)
