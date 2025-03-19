from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.secrets.Vault import Vault


YQL_QUERY = 'SELECT * FROM `//home/cloud_analytics/test/guide_table`'


def main():
    with open('/home/shasto4ka/.yav/token', 'r') as f:
        token = f.read().replace('\n', '')

    Vault(token=token).get_secrets()
    yql_adapter = YQLAdapter()
    result = yql_adapter.run_query_to_pandas(YQL_QUERY)
    print('\nI\'ve reached', result.iloc[0, 0])


if __name__ == '__main__':
    main()
