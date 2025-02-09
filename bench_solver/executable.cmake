target_link_libraries(${TARGET_NAME} PRIVATE solver)

add_custom_target(${TARGET_NAME}_sh
        COMMAND cpupower frequency-set --governor performance
        && chrt -f 99 "${TARGET_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}"
        WORKING_DIRECTORY "${TARGET_RUNTIME_OUTPUT_DIRECTORY}"
)
add_dependencies(${TARGET_NAME}_sh ${TARGET_NAME})
