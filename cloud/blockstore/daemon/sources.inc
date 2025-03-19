SRCS(
    main.cpp
)

IF (BUILD_TYPE != "PROFILE" && BUILD_TYPE != "DEBUG")
    SPLIT_DWARF()
ENDIF()
