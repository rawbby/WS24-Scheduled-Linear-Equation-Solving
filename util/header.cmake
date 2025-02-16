if (MSVC)
    # ensure __VA_OPT__ is supported (C++20 feature)
    target_compile_options(${TARGET_NAME} INTERFACE "/Zc:preprocessor")
endif ()

if (UNIX AND NOT APPLE)
    target_link_libraries(${TARGET_NAME} INTERFACE -lpthread)
endif ()
