from clan_tools.data_adapters.InterFaxAdapter import InterFaxAdapter
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
import logging.config
import dask.dataframe as dd
import pandas as pd
import numpy as np
import click

logger = logging.getLogger(__name__)
logging.config.dictConfig(default_log_config)


@click.command()
@click.option('--result_table_path')
def main(result_table_path):
    chyt_adapter = ClickHouseYTAdapter()
    ifax_adapter = InterFaxAdapter()

    clients_df = chyt_adapter.execute_query('''
        --sql
        SELECT DISTINCT
            passport_id,
            inn
        FROM "//home/cloud_analytics/import/balance/balance_persons"
        WHERE (inn IS NOT NULL ) AND (passport_id IS NOT NULL)
        LIMIT 10
        --endsql
    ''', to_pandas=True)

    logger.info(f'shape = {clients_df.shape}')

    def get_spark_fields(inn):
        company_extended = ifax_adapter.get_extended_report(str(inn))
        revenue_dict = ifax_adapter.latest_year_revenue(company_extended)
        if company_extended is not None:
            revenue_dict['company_size'] = ifax_adapter.get_company_size(
                company_extended)
            revenue_dict['company_name'] = ifax_adapter.get_company_name(
                company_extended)
        else:
            revenue_dict['company_size'] = np.nan
            revenue_dict['company_name'] = np.nan
        return revenue_dict

    clients_dd = dd.from_pandas(clients_df, npartitions=10)

    revenues = clients_dd['inn'].apply(get_spark_fields, meta=('inn', 'int'))

    revenues_cols = revenues.compute()

    revenues = pd.DataFrame(revenues_cols.values.tolist())
    clients_df = pd.concat([clients_df, revenues], axis=1)

    type_map = {'object': 'string', 'float64': 'double', 'int64': "int64"}
    schema = clients_df.dtypes.astype(str).map(type_map).to_dict()
    clients_df = clients_df.where(clients_df.notnull(), None)
    YTAdapter().save_result(result_table_path,
                            schema, clients_df, append=False)


if __name__ == '__main__':
    main()
