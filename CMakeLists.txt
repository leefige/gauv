# -*- mode:cmake -*-
if(NOT UNIX)
    message(WARNING "Unsupported operating system")
endif()

set(ANTLR_JAR_LOCATION
    ${PROJECT_SOURCE_DIR}/lib/antlr-4.9.2-complete.jar
)

set(GENERATED_FRONTEND_SRC
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCLexer.cpp 
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCParser.cpp
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCParserListener.cpp
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCParserVisitor.cpp
    # TODO: move these to src dir
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCParserBaseListener.cpp
    ${PROJECT_SOURCE_DIR}/src/frontend/MPCParserBaseVisitor.cpp
)

foreach(src_file ${GENERATED_FRONTEND_SRC})
    set_source_files_properties(
        ${src_file}
        PROPERTIES
        GENERATED TRUE
        )
endforeach(src_file ${GENERATED_FRONTEND_SRC})

add_custom_target(GenerateParser DEPENDS ${GENERATED_FRONTEND_SRC})
add_custom_command(OUTPUT ${GENERATED_FRONTEND_SRC}
    COMMAND 
    ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/src/frontend/
    COMMAND
    ${PROJECT_SOURCE_DIR}/tools/generate.sh
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS ${PROJECT_SOURCE_DIR}/src/grammar/TLexer.g4 ${PROJECT_SOURCE_DIR}/src/grammar/TParser.g4
)

include_directories(
    # cpp runtime
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/atn
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/dfa
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/misc
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/support
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/tree
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/include/antlr4-runtime/utf8
    # generated frontend
    ${PROJECT_SOURCE_DIR}/src/frontend
)

set(SRC 
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${GENERATED_FRONTEND_SRC}
)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set (flags_1 "-Wno-overloaded-virtual")
else()
    set (flags_1 "-MP /wd4251")
endif()

foreach(src_file ${SRC})
    set_source_files_properties(
        ${src_file}
        PROPERTIES
        COMPILE_FLAGS "${COMPILE_FLAGS} ${flags_1}"
    )
endforeach(src_file ${SRC})

add_executable(mpc-parser
    ${SRC}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(antlr4-demo PRIVATE "/MT$<$<CONFIG:Debug>:d>")
endif()

add_dependencies(mpc-parser GenerateParser)

target_link_libraries(mpc-parser
    # TODO: define static & dynamic link vars
    ${PROJECT_SOURCE_DIR}/runtime/Cpp/lib/libantlr4-runtime.a
)

install(TARGETS mpc-parser
    DESTINATION "share" 
    COMPONENT dev 
)
