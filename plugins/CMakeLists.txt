
include_directories(../core)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(SRCS
	plugin.cpp
	Segmentation/segmentation.cpp
)

add_library(segmentation SHARED Segmentation/segmentation.cpp)
target_link_libraries(segmentation core)

add_library(segmentationTest SHARED SegmentationTest/segmentation.cpp)
target_link_libraries(segmentationTest core)

add_library(entropy SHARED Entropy/Entropy.cpp)
target_link_libraries(entropy core)

add_library(median SHARED Median/Median.cpp)
target_link_libraries(median core)
