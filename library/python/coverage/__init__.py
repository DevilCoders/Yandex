# This module is registered as PY_CONSTRUCTOR and will be imported before program's main()

import os


COVERAGE_INSTANCE = [None]


def start_coverage_tracing(filename_prefix):
    import atexit
    import sys
    import coverage

    # Already started
    if COVERAGE_INSTANCE[0]:
        return

    filename_prefix = filename_prefix.format(
        bin=os.path.basename(sys.executable),
        python_ver=sys.version_info[0],
    )

    env_val = os.environ.get('YA_DEBUG_COVERAGE') or ''
    if env_val in ('1', 'yes'):
        debug = ['pid', 'trace', 'sys', 'config']
    elif env_val not in ('', '0', 'no'):
        debug = env_val.split(",")
    else:
        debug = []

    cov = coverage.Coverage(
        data_file=filename_prefix,
        concurrency=['multiprocessing', 'thread'],
        auto_data=True,
        branch=True,
        debug=debug,
    )

    cov.start()
    COVERAGE_INSTANCE[0] = cov

    atexit.register(stop_coverage_tracing)


def stop_coverage_tracing():
    cov = COVERAGE_INSTANCE[0]
    if cov:
        cov.stop()
        COVERAGE_INSTANCE[0] = None


def init():
    # Setup coverage collection
    if 'PYTHON_COVERAGE_PREFIX' in os.environ:
        import library.python.coverage
        library.python.coverage.start_coverage_tracing(os.environ['PYTHON_COVERAGE_PREFIX'])
