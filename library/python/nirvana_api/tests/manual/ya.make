OWNER(mihajlova)

PY2TEST()

TAG(ya:manual)

# 5 mins
TIMEOUT(300)

TEST_SRCS(
    base.py
    highlevel_api_test.py
    workflow_instance_test.py
)

PEERDIR(library/python/nirvana_api)

END()
