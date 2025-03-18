IF (NOT OS_WINDOWS)
    PEERDIR(
        library/python/statface_client
    )
ENDIF()

SET(PREFIX library/python/statface_client/tests/common)

TEST_SRCS(
    ${PREFIX}/conftest.py
    ${PREFIX}/test_client_config.py
    ${PREFIX}/test_nested_dict.py
    ${PREFIX}/test_report_config.py
    ${PREFIX}/test_serializing.py
    ${PREFIX}/test_tools.py
    ${PREFIX}/test_upload_checker.py
    ${PREFIX}/test_utils.py
)

DATA(
    arcadia/${PREFIX}/buggy_config.yaml
    arcadia/${PREFIX}/config.yaml
    arcadia/${PREFIX}/report_example.tskv
    arcadia/${PREFIX}/statface_config.yaml
    arcadia/${PREFIX}/user_config.yaml
)

