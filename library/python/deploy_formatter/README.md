# Logs formatter for Yandex.Deploy and standard Python logging module

This formatter is recommended to use if you want Ya.Deploy to collect your logs in structured format.
It's more simple than using Unified Agent GRPC interface as it's just prepares JSON data for stdout.

Possible way to use:

    import sys
    import logging
    from library.python.deploy_formatter import DeployFormatter

    formatter = DeployFormatter()

    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    handler.setFormatter(formatter)

    log = logging.getLogger('')
    log.setLevel(logging.DEBUG)
    log.addHandler(handler)

    some_child_log = logging.getLogger('my.app.log')

    try:
        raise RuntimeError("Oops")
    except Exception as e:
        some_child_log.exception("This is shiny exception with traceback and all attributes set as needed: %s", e)
