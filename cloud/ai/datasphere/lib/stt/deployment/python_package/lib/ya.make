PY3_LIBRARY()

OWNER(eranik)

SRCDIR(cloud/ai/datasphere/lib/stt/deployment/python_package/stt_deployment)

PEERDIR()

NO_COMPILER_WARNINGS()

PY_SRCS(
    NAMESPACE stt_deployment
    __init__.py
    deployment_builder.py
)

END()
