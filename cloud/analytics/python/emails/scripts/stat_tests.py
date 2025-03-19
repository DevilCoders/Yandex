
import numpy as np
import scipy.stats as sps
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
import click
import logging.config
from clan_tools.logging.logger import default_log_config
import json

np.seterr(all='raise')
logging.config.dictConfig(default_log_config)


@click.command()
@click.option('--dataset_path')
@click.option('--result_path')
def main(dataset_path, result_path):

    cons_by_ba_program = ClickHouseYTAdapter().execute_query(f'''
        --sql
        SELECT
            program_name,
            email,
            stream_name,
            toStartOfMonth(toDate("first_add_to_nurture")) as month,
            min(first_add_to_nurture) as first_date_after,
            if(isNotNull(first_date_after),
                sumIf(paid, months_after_first_add_to_nurture in ('0', '1', '2', '3', '4', '5')
                    AND is_consumption_after=1)/dateDiff('day', toDate(first_date_after), 
                toDate(now())), 
                0) AS avg_daily_cons_after
        FROM "{dataset_path}" 
        GROUP BY  program_name,
                email,
                stream_name,
                month
        --endsql
    ''', to_pandas=True)

    avg_daily_cons_after = (
        cons_by_ba_program
        .groupby(['program_name', 'month', 'stream_name'])['avg_daily_cons_after']
        .mean()
        .unstack()
        .reset_index()
    )
    avg_daily_cons_after['Stream Impact Per Email'] = avg_daily_cons_after['test'] - \
        avg_daily_cons_after['control']
    avg_daily_cons_after = (
        avg_daily_cons_after
        .rename(columns={'control': 'Control Rev. Per Email',
                         'test': 'Test Rev. Per Email'})
    )

    def compare(df):
        pval = np.nan
        try:
            pval = sps.ttest_ind(
                df[df.stream_name == 'control'].avg_daily_cons_after.fillna(0),
                df[df.stream_name == 'test'].avg_daily_cons_after.fillna(0),
                equal_var=False).pvalue
        except:
            pass
        return pval

    pvals = cons_by_ba_program.groupby(
        ['program_name', 'month']).apply(compare).reset_index()
    pvals.columns = ['program_name', 'month', 'p-value']

    emails_count = (
        cons_by_ba_program
        .groupby(['program_name', 'month', 'stream_name'])['email']
        .nunique()
        .unstack()
        .reset_index()
        .rename(columns={'control': 'Control Emails',
                         'test': 'Test Emails'})
    )

    stream_impact_df = (
        emails_count
        .merge(avg_daily_cons_after, on=['program_name', 'month'])
        .merge(pvals, on=['program_name', 'month'])
    )
    stream_impact_df['Stream Impact Total'] = stream_impact_df['Stream Impact Per Email'] * \
        stream_impact_df['Test Emails']
    stream_impact_df['Stream Cost'] = stream_impact_df['Stream Impact Per Email'] * \
        stream_impact_df['Control Emails']
    stream_impact_df = (
        stream_impact_df
        .rename(columns={'program_name': 'Program Name',
                         'month': 'First Add To Nurture'
                         })
    )

    YTAdapter().save_result(result_path, schema=None,
                            df=stream_impact_df, append=False)

    with open('output.json', 'w') as f:
        json.dump({"tables_path": result_path}, f)


if __name__ == "__main__":
    main()
