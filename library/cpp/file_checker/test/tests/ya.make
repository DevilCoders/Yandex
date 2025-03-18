OWNER(velavokr)

IF (NOT OS_WINDOWS)
    #    IF(CMAKE_BUILD_TYPE MATCHES "[rR]elease")
    IF ("${HARDWARE_TYPE}" MATCHES "x86_64" OR "${HARDWARE_TYPE}" MATCHES "ia64")
        PY2TEST()

        TEST_SRCS(file_checker_test.py)

        DEPENDS(library/cpp/file_checker/test)



        END()
    #    ENDIF ()
    ENDIF()
ENDIF()
