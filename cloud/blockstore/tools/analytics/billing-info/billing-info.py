import os, io
import pandas as pd
import requests, ipaddress
from typing import List, Set, Callable


class Kikhouse():
    def __init__(self, endpoint, **kwargs):
        session = requests.Session()
        if "token" in kwargs and kwargs["token"] != "":
            session.headers.update({'Authorization': f'OAuth {kwargs["token"]}'})
        self.session = session
        self.endpoint = endpoint

    def get(self, body) -> requests.Response:
        # TODO fix verify=False
        return self.session.post(self.endpoint, data=body, verify=False)


class KikhouseClient():
    def __init__(self, kikhouse: Kikhouse, ydb_prefix: str):
        self.kikhouse = kikhouse
        self.ydb_prefix = ydb_prefix

    # TODO: shit, use only on trusted input
    @staticmethod
    def list_escaper(lst: List[str]) -> str:
        escaper = lambda s: s.replace("\\", "\\\\").replace("'", "\\'")
        joined = ", ".join(
            [f"'{escaper(item)}'" for item in lst]
        )
        return joined

    @staticmethod
    def _to_df(resp: requests.Response):
        # TODO: log error and use empty dataset replacement
        if resp.status_code == 200:
            return pd.read_csv(io.BytesIO(resp.content), encoding='utf8')
        resp.raise_for_status()
        return pd.DataFrame()

    @staticmethod
    def empty_df(*columns): return pd.DataFrame(columns={*columns})

    def ba_by_cloud(self, clouds: List[str]) -> pd.DataFrame:
        clouds_str = self.list_escaper(clouds)
        query = f'''
            SELECT
                service_instance_id AS cloud_id,
                billing_account_id
            FROM ydbTable('{self.ydb_prefix}/billing/hardware/default/billing/meta/service_instance_bindings/bindings_billing_account_id_idx')
            WHERE service_instance_id IN (
                {clouds_str}
            )
            AND service_instance_type = 'cloud'
            ORDER BY cloud_id
            FORMAT CSVWithNames
        '''
        return self._to_df(self.kikhouse.get(query))

    def billing_account_info(self, accounts: List[str]) -> pd.DataFrame:
        accounts_str = self.list_escaper(accounts)
        query = f'''
            SELECT
                id, name, created_at,
                JSONExtractBool(metadata, 'verified') AS verified,
                usage_status, person_type,
                JSONExtractString(metadata, 'idempotency_checks', 5) AS email
            FROM ydbTable('{self.ydb_prefix}/billing/hardware/default/billing/meta/billing_accounts')
            WHERE id IN (
                {accounts_str}
            )
            ORDER BY id
            FORMAT CSVWithNames
        '''
        return self._to_df(self.kikhouse.get(query))


def batched_apply(lst: List, size: int, func: Callable):
    res = pd.DataFrame()
    for i in range(0, len(lst), size):
        batch = lst[i:i+size]
        res_batch = func(batch)
        res = res.append(res_batch)
    return res


def handle():
    clouds_path = os.getenv("CLOUDS_PATH", "/tmp/clouds.csv")
    endpoint = os.getenv("YC_KIKHOUSE_ENDPOINT", "https://kikhouse.svc.kikhouse.bastion.cloud.yandex-team.ru")
    token = os.getenv("YC_BASTION_TOKEN", "")
    ydb_prefix = os.getenv("YC_YDB_ROOT", "/global")

    kikhouse = Kikhouse(endpoint, token=token)
    client = KikhouseClient(kikhouse, ydb_prefix)
    with open(clouds_path) as f:
        all_clouds = pd.read_csv(f, names=['cloud_id', 'disk_cnt'], skiprows=1)

    kikhouse_limit = 100
    cloud_with_ba = batched_apply(all_clouds["cloud_id"].unique().tolist(), kikhouse_limit, lambda clouds: client.ba_by_cloud(clouds))

    ba_status = batched_apply(cloud_with_ba['billing_account_id'].unique().tolist(), kikhouse_limit, lambda accounts: client.billing_account_info(accounts))
    ba_status.rename(columns={"id": "billing_account_id"}, inplace=True)

    res = all_clouds\
        .merge(cloud_with_ba, on='cloud_id', how='left')\
        .merge(ba_status, on='billing_account_id', how='left')\
    [['billing_account_id', 'cloud_id', 'disk_cnt', 'name', 'created_at', 'verified', 'usage_status', 'person_type', 'email']]

    res.to_csv('/tmp/clouds_res.csv')


if __name__ == '__main__':
    handle()

