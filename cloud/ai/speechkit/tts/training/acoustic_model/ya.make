OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    acoustic_model.py
    aligner.py
    discriminator.py
    loss.py
    train_module.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    data
    export
    prior_attn
)
