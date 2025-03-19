OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    acoustic_to_onnx.py
    export_acoustic_resources.py
    features_extractor_to_onnx.py
)

NO_CHECK_IMPORTS()

END()
