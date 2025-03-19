import os
from typing import List

from transfer_dto import TransferDTO


class RestartSetting:
    def __init__(self, weekday: int, hour_from: int, hour_to: int) -> None:
        self.weekday = weekday
        self.hour_from = hour_from
        self.hour_to = hour_to


class Config:
    def __init__(self) -> None:
        pass

    def transfers(self) -> List[TransferDTO]:
        return [
            TransferDTO("dttk6fldmo91i2ef71j3", "crm-partition1-mysql-lbk"),
            TransferDTO("dttsn97lhkeh38tnmohs", "crm-partition2-mysql-lbk"),
            TransferDTO("dtttbu3q8obf7ordded1", "support-chats-pg-lbk"),
            TransferDTO("dtttbu3q8obf7orddeda", "mdb-pg")
        ]

    def dt_api_host(self) -> str:
        return str(os.getenv('DT_API_HOST'))

    def iam_token(self, context) -> str:
        return context.token["access_token"]

    def reload_time(self) -> RestartSetting:
        return RestartSetting(3, 8, 9)  # Thursday from 8am to 9 am UTC.
