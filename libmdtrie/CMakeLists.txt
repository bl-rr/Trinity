include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(mdtrie INTERFACE)
target_include_directories(mdtrie INTERFACE src/)
set(MDTRIE_LIBS "${HEAP_MANAGER_LIBRARY}" "${CMAKE_DL_LIBS}" "mdtrie")

add_compile_options(-mbmi2 -g)

add_executable(microbench
        bench/microbench.cpp)

add_executable(example
        bench/example.cpp)

add_executable(disk_example
        bench/disk_example.cpp)

add_executable(serialize
        bench/serialize.cpp)

add_executable(deserialize
        bench/deserialize.cpp)

target_link_libraries(microbench ${MDTRIE_LIBS})
target_link_libraries(example ${MDTRIE_LIBS})
target_link_libraries(disk_example ${MDTRIE_LIBS})
target_link_libraries(serialize ${MDTRIE_LIBS})
target_link_libraries(deserialize ${MDTRIE_LIBS})