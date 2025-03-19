import vh
import pandas as pd
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import python_dl_base
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src: bool = False,  is_prod: bool = False, with_start: bool = False) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    package = get_package(package_path='ml/st_attachments_to_text', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    python_dl_base(
            script=package,
            run_command='python3 $SOURCE_CODE_PATH/scripts/data.py',
            user_requested_secret = 'iam_token',
            pip=['clan_tools', 'PyJWT', 'scikit-learn',  'yandex-yt-yson-bindings'],
    )

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid='9b59113f-25a4-4905-bebe-ba21ba6d48bf',
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
