PY3_LIBRARY()

OWNER(eranik)

SRCDIR(cloud/ai/datasphere/lib/stt/text_templates/python_package/text_templates)

PEERDIR(
)

NO_COMPILER_WARNINGS()

PY_SRCS(
    NAMESPACE text_templates
    __init__.py
    generation.py
    preparation.py
    validation.py
)

END()
