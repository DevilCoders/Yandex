from datetime import datetime

from dt_api import DtApi
from function_config import Config, RestartSetting


def need_reload(now: datetime, reload_setting: RestartSetting) -> bool:
    return (
        now.weekday() == reload_setting.weekday
        and now.hour >= reload_setting.hour_from
        and now.hour <= reload_setting.hour_to
    )


def handler(event, context):
    config = Config()
    api_worker = DtApi(config.dt_api_host(), config.iam_token(context))

    response = []
    for transfer in config.transfers():
        status_before = api_worker.status(transfer.id)

        action = "None"
        if need_reload(datetime.utcnow(), config.reload_time()):
            api_worker.deactivate(transfer.id)
            action = "Deactivate"
        else:
            need_activation = status_before == 'DONE'
            if need_activation:
                api_worker.activate(transfer.id)
            action = "Activate"

        status_after = api_worker.status(transfer.id)

        transfer_status = {
            'transfer': {
                'id': transfer.id,
                'name': transfer.name
            },
            'action': action,
            'statusBefore': status_before,
            'statusAfter': status_after
        }
        response.append(transfer_status)

    return {
        'statusCode': 200,
        'body': {
            'activationResponse': response
        }
    }
