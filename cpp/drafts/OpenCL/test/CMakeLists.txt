file(GLOB test_OpenCL_SOURCE_FILES FILES test_*.cpp)

list(APPEND test_deps )
foreach (file ${test_OpenCL_SOURCE_FILES})
  get_filename_component(filename "${file}" NAME_WE)
  sara_add_test(
    NAME ${filename}
    SOURCES ${file}
    DEPENDENCIES ${DO_Sara_LIBRARIES} ${OpenCL_LIBRARY}
    FOLDER OpenCL)
  target_compile_definitions(${filename} PRIVATE CL_SILENCE_DEPRECATION)
endforeach ()
