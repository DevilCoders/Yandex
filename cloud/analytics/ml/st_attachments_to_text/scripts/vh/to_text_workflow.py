import vh
import pandas as pd
from clan_tools.logging.logger import default_log_config
import logging.config
from functools import partial
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
from clan_tools.vh.operations import spark_op, python_dl_base, run_yql_script
from clan_tools.utils.dict import DictObj
from clan_tools.vh.operations import run_small_job_op
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
    
    files = python_dl_base(
            script=package,
            run_command='python3 $SOURCE_CODE_PATH/scripts/collect_attachment.py',
            pip=['clan_tools', 'yandex-yt-yson-bindings', 'dataclasses'],
            user_requested_secret = 'robot-clanalytics-yav'
        )

    images = vh.op(id='7a2719c4-7b5a-4837-ac87-2662d43fd084')(files)

    results = partial(
        vh.op(id='847b6e3c-2b49-4abb-9b83-fdf7b54ada11'),
        strategy='FullOcrMultihead',
        langs='rus, eng',
        config='translate/ocr.cfg',
        ocr_data_revision=6518576,
        code_revision=6518576,
        ttl=360,
        max_ram=15120,
        max_disk=40000,
        arcadia_revision_run_ocr=6518576,
        cpu_guarantee=5600
        )(images)
    jsons = vh.op(id='4764c2b0-8f0e-4ed7-9021-5a124795108f')(results)
    # archive = zip_op(jsons).output

    wtite_to_yt = python_dl_base(
        _inputs={'data':jsons},
        script=package,
        run_command='python3 $SOURCE_CODE_PATH/scripts/write_to_yt.py',
        user_requested_secret = 'robot-clanalytics-yav',
        pip=['clan_tools', 'yandex-yt-yson-bindings', 'pymorphy2', 'dataclasses'])

    # with vh.wait_for(wtite_to_yt):
    #     run_yql_script(path='src/st_attachments_to_text/yt/dates.sql')

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('166324b6-4a6a-4a46-b35b-89f8b8c60f40' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
