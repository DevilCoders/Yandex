OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    train_module.py
    trainer.py
    utils.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    callbacks
    data
    loggers
    lr_schedulers
    models
    optimizers
)
