PY3_LIBRARY()

OWNER(
    d-kruchinin
    hotckiss
)

PEERDIR(
    contrib/python/boto3
    contrib/python/retry
    contrib/python/PyJWT
)

PY_SRCS(
    config.py
    helpers.py
    random_name.py
    retryable.py
    resources/adjectives.py
    resources/animals.py
    resources/colors.py
    process/base_process.py
    process/job_process.py
    process/cli_process.py
    process/function_process.py
    release_creator.py
    release_manager.py
    thread_logger.py
    renewable_iam_token_generator.py
    jwt_generator.py
    key_management_service.py
)

END()
