cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

# instead of looking for the usual main folder, tell ESP-IDF to look for those two.
set(EXTRA_COMPONENT_DIRS core application)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(TuTo)