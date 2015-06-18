include_directories(
  ${DO_Sara_INCLUDE_DIR}
  ${DO_Sara_ThirdParty_DIR}/eigen
  ${DO_Sara_ThirdParty_DIR}/flann/src/cpp)

if (DO_USE_FROM_SOURCE)
  get_property(DO_Sara_KDTree_ADDED GLOBAL PROPERTY _DO_Sara_KDTree_INCLUDED)
  if (NOT DO_Sara_KDTree_ADDED)
    do_glob_directory(${DO_Sara_SOURCE_DIR}/KDTree)
    do_create_common_variables("KDTree")
    do_generate_library("KDTree")
    target_link_libraries(DO_Sara_KDTree flann_cpp_s)
  endif ()
endif ()