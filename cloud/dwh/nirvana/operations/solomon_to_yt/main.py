import asyncio

import nirvana.job_context as nv

from cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation import run_operation
from cloud.dwh.utils.log import setup_logging


def main():
    setup_logging(debug=nv.context().get_parameters().get('debug-logging', False))
    asyncio.run(run_operation())


if __name__ == '__main__':
    main()
