find_package(DO_Sara COMPONENTS Core Graphics ImageIO ImageProcessing REQUIRED)

macro (sara_add_example example)
  add_executable(${example} ${example}.cpp)
  set_target_properties(${example} PROPERTIES
                        COMPILE_FLAGS ${SARA_DEFINITIONS})
  target_link_libraries(${example} ${DO_Sara_LIBRARIES})
  set_property(
    TARGET ${example} PROPERTY
    FOLDER "Examples/Sara/ImageProcessing")
endmacro()

sara_add_example(image_processing_example)
sara_add_example(infinite_image_example)
sara_add_example(gemm_based_convolution_example)
