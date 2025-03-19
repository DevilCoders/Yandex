# Tokens for connection to different services
from clan_tools.secrets.Vault import Vault
Vault().get_secrets()

# Connector to YT
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
ch_adapter = ClickHouseYTAdapter()

dm_yc_cons_path = '\"//home/cloud-dwh/data/prod/cdm/dm_yc_consumption\"'

service_name_sql = '''
    select
        sku_service_group as service_group
    from
        {table_path}
    group by
        sku_service_group
'''.format(table_path=dm_yc_cons_path)

df_service_group = ch_adapter.execute_query(query=service_name_sql, to_pandas=True)

print(ch_adapter.execute_query(query=service_name_sql, to_pandas=True))

# print(service_name_sql)