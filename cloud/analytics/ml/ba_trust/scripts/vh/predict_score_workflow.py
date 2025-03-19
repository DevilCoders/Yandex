import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_small_job_op, python_dl_base, run_yql_script
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src,  is_prod, with_start):

    package = get_package(
        package_path='ml/ba_trust',
        local_script=local_src,
        files_to_copy=['src/', 'scripts/']
    )

    test_dataset = run_yql_script(path='src/ba_trust/yql/prepare_test_dataset.sql', _name='Test Dataset')

    with vh.wait_for(test_dataset):
        predictor = python_dl_base(
            script=package,
            run_command='python3 $SOURCE_CODE_PATH/scripts/predict_scores.py',
            pip=['clan_tools', 'statsmodels', 'scikit-learn',  'yandex-yt-yson-bindings'],
        )

    with vh.wait_for(predictor):
        run_small_job_op(
            _name='Test if updated',
            input=package,
            max_ram=128,
            script='scripts/test_scores_updating.py',
            yql_token='robot-clanalytics-yql'
            )

    vh.run(
        wait=False,
        start=with_start,
        workflow_guid='29e899a7-747a-4102-9c02-011c4a697ac0' if is_prod else None,
        label='Predict Scores',
        quota='coud-analytics', backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
