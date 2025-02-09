if (MSVC)
    # ensure __VA_OPT__ is supported (C++20 feature)
    target_compile_options(${TARGET_NAME} PRIVATE "/Zc:preprocessor")
endif ()
