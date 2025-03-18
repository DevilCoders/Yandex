EXECTEST()

OWNER(gluk47)

RUN(
    NAME zdocid
    STDIN input1.txt
    STDOUT ${TEST_OUT_ROOT}/output.txt
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/output.txt
    doc_handle_decode
)

RUN(
    NAME with-route
    STDIN input2.txt
    STDOUT ${TEST_OUT_ROOT}/output.txt
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/output.txt
    doc_handle_decode
)

RUN(
    NAME no-docids
    STDIN input3.txt
    STDOUT ${TEST_OUT_ROOT}/output.txt
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/output.txt
    doc_handle_decode
)

TEST_CWD(tools/doc_handle_decode/ut)

DEPENDS(tools/doc_handle_decode)

END()
