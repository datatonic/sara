find_package(DO_Sara COMPONENTS Graphics Features ImageProcessing REQUIRED)

file(GLOB test_features_SOURCE_FILES FILES *.cpp)
foreach (file ${test_features_SOURCE_FILES})
  get_filename_component(filename "${file}" NAME_WE)
  sara_add_test(
    NAME ${filename}
    SOURCES ${file}
    DEPENDENCIES ${DO_Sara_LIBRARIES}
    FOLDER Features)
endforeach ()


find_package(HDF5 COMPONENTS CXX REQUIRED)
target_include_directories(test_features_hdf5 PRIVATE ${HDF5_INCLUDE_DIRS})
target_link_libraries(test_features_hdf5 PRIVATE ${HDF5_LIBRARIES})
