find_package(DO_Sara COMPONENTS Core Features Match REQUIRED)
find_package(HDF5 COMPONENTS CXX REQUIRED)

file(GLOB test_match_SOURCE_FILES FILES *.cpp)

foreach (file ${test_match_SOURCE_FILES})
  get_filename_component(filename "${file}" NAME_WE)
  sara_add_test(
    NAME ${filename}
    SOURCES ${file}
    DEPENDENCIES ${DO_Sara_LIBRARIES}
    FOLDER Match)
endforeach ()
