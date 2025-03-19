import vh
from startrek_client import Startrek
from yt import wrapper as yt

from cloud.dwh.nirvana.vh.config.base import ST_TOKEN
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "RAW StarTrek Personal Data for removal to YT"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"
SOURCE_SYSTEM_NAME = "startrek"

YT_TABLE_PATH = "//home/cloud_analytics/dwh/stg/iam_anonimize"
ST_PD_COMPONENT_ID = "59866"
ST_PD_QUEUE = "CLOUDSUPPORT"
ST_BASE_URL = "https://st-api.yandex-team.ru"


@vh.lazy(object, st_token=vh.Secret,  current_datetime=vh.mkinput(vh.TextFile))
def get_personal_data_for_removal(st_token, current_datetime):
    client = Startrek(useragent='yc-dwh-nirvana', base_url=ST_BASE_URL, token=st_token.value)
    issues = client.issues.find(
        filter={'queue': ST_PD_QUEUE, 'components': [ST_PD_COMPONENT_ID], },
        per_page=50
    )
    result = []
    for issue in issues:
        if issue.cloudId or issue.uid:
            result.append(dict(cloud_id=issue.cloudId, passport_uid=issue.uid))
    return result


@vh.lazy(object, personal_data=vh.mkinput(object), yt_token=vh.Secret)
def write_data_to_yt(personal_data, yt_token):
    yt.config["proxy"]["url"] = "hahn"
    yt.config["token"] = yt_token.value
    yt.create(
        type="table",
        path=YT_TABLE_PATH,
        force=True,
        attributes={
            "schema": [
                {'name': 'cloud_id', 'type': 'string'},
                {'name': 'passport_uid', 'type': 'string'},
            ]
        }
    )
    yt.write_table(YT_TABLE_PATH, personal_data, format=yt.JsonFormat(attributes={"encode_utf8": False}))


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        op_get_current_datetime = vh.op(id="87c1dc43-461e-4de3-8f26-c0d1406165b6")
        cur_datetime = op_get_current_datetime(
            date="today",
            format="%Y-%m-%dT%H:%M:%S",
            utc=True
        )
        pd_result = get_personal_data_for_removal(current_datetime=cur_datetime, st_token=ST_TOKEN)
        write_data_to_yt(personal_data=pd_result, yt_token=YT_TOKEN)

    # run_config["workflow_guid"] = "262db0ef-e9d3-4330-9260-af137da74444"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["raw", SOURCE_SYSTEM_NAME]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}raw/yt/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
