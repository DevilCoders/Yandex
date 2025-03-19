import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import python_dl_base, run_yql_script
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

    train_dataset = run_yql_script(path='src/ba_trust/yql/prepare_train_dataset.sql', _name='Train Dataset')

    with vh.wait_for(train_dataset):
        python_dl_base(
            script=package,
            run_command='python3 $SOURCE_CODE_PATH/scripts/train_model.py',
            pip=['clan_tools', 'statsmodels', 'yandex-yt-yson-bindings', 'scikit-learn', 'category_encoders'],
        )

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid='287cea52-a9a1-4a0d-9c5b-eaf905d930ec' if is_prod else None,
        label='Train Trust model',
        quota='coud-analytics', backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
