PY3TEST()

OWNER(hotckiss)

PEERDIR(
    cloud/ai/nirvana/nv_launcher_agent/lib
)

TEST_SRCS(
    process/cli_process_tests.py
    process/function_process_tests.py
    merge_layers_test.py
)

END()
