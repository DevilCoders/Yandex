OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    nirvana_train_acoustic_model.py
    nirvana_train_e2e_model.py
    nirvana_train_vocoder.py
    train_acoustic_model.py
    train_e2e_model.py
    train_vocoder.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    acoustic_model
    core
    e2e_model
    pipeline
    vocoder
)
