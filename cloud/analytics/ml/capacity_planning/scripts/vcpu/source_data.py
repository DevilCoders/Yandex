import json
import click
import logging.config
from textwrap import dedent
from datetime import datetime, timedelta
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--workdir')
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def main(workdir: str, is_prod: bool, rebuild: bool) -> None:
    """Preprocess raw data imported from Solomon

    Args:
        workdir (str): directory to work in (might be directory of prod or preprod resources environment)
        is_prod (bool): if set to False workflow works with test tables and sources (used for debug)
        rebuild (bool): if set to True rebuild all historical data, if not just append new part
    """
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()

    date_fmt = '%Y-%m-%d'
    postfix = '' if is_prod else '_test'
    source = f'{workdir}/1d'
    target = f'{workdir}/all{postfix}'

    target_table_name = target.split('/')[-1]
    target_table_folder = '/'.join(target.split('/')[:-1])
    source_tables = yt_adapter.yt.list(source)

    if target_table_name in yt_adapter.yt.list(target_table_folder):
        if is_prod and not rebuild:
            last_rep_date = yql_adapter.execute_query(f'SELECT Max(rep_date) FROM `{target}`', True).iloc[0, 0]
            start_date = (datetime.strptime(last_rep_date, date_fmt) + timedelta(days=1)).strftime(date_fmt)
        elif rebuild:
            yt_adapter.yt.remove(target)
            start_date = min(source_tables)
        else:
            yt_adapter.yt.remove(target)
            start_date = (datetime.today() - timedelta(days=5)).strftime(date_fmt)
    else:
        if is_prod:
            start_date = min(source_tables)
        else:
            min_available_date = min(source_tables)
            start_date = max(min_available_date, (datetime.today() - timedelta(days=5)).strftime(date_fmt))

    yesterday_date = (datetime.today() - timedelta(days=1)).strftime(date_fmt)
    end_date = min(yesterday_date, max(source_tables))

    if start_date <= end_date:
        prev_table_query = f'''
            SELECT `rep_date`, `platform`, `zone`, `used_wop`, `used`, `total`
            FROM `{target}`
            WHERE `rep_date` < '{start_date}'
            UNION ALL
        ''' if target_table_name in yt_adapter.yt.list(target_table_folder) else ''

        query = yql_adapter.execute_query(dedent(f'''
            PRAGMA yt.InferSchema = '1';
            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            $source_folder = '{source}';
            $result_table = '{target}';
            $preempts_list = ['preemptible_shared_cores_used', 'preemptible_exclusive_cores_used'];
            $safecast = ($num) -> (IF(String::IsAsciiAlpha($num), Null, Cast($num AS Float)));

            $base =
                SELECT
                    TableName() as `rep_date`,
                    `metric`,
                    `platform`,
                    `zone_id` AS `zone`,
                    $safecast(`value`) AS `value`
                FROM Range($source_folder, '{start_date}', '{end_date}')
                WHERE `zone_id` != 'all_zones'
            ;

            $total =
                SELECT `rep_date`, `platform`, `zone`, Math::Round(Min(`value`), -2) AS `total`
                FROM $base
                WHERE `metric` = 'cores_total'
                GROUP BY `rep_date`, `platform`, `zone`
            ;

            $free =
                SELECT `rep_date`, `platform`, `zone`, Math::Round(Min(`value`), -2) AS `free`
                FROM $base
                WHERE `metric` = 'cores_free'
                GROUP BY `rep_date`, `platform`, `zone`
            ;

            $preempts =
                SELECT `rep_date`, `platform`, `zone`, Math::Round(Median(`value`), -2) AS `preempts`
                FROM $base
                WHERE `metric` in $preempts_list
                GROUP BY `rep_date`, `platform`, `zone`
            ;

            INSERT INTO $result_table WITH TRUNCATE
                SELECT `rep_date`, `platform`, `zone`, `used_wop`, `used`, `total`
                FROM (
                    {prev_table_query}
                    SELECT
                        Coalesce(t.`rep_date`, f.`rep_date`, p.`rep_date`) AS `rep_date`,
                        Coalesce(t.`platform`, f.`platform`, p.`platform`) AS `platform`,
                        Coalesce(t.`zone`, f.`zone`, p.`zone`) AS `zone`,
                        Math::Round(`total` - `free` - `preempts`, -2) AS `used_wop`,
                        Math::Round(`total` - `free`, -2) AS `used`,
                        Math::Round(`total`, -2) AS `total`
                    FROM $total AS t
                    FULL JOIN $free As f
                        ON t.`rep_date` = f.`rep_date` AND t.`platform` = f.`platform` AND t.`zone` = f.`zone`
                    FULL JOIN $preempts AS p
                        ON t.`rep_date` = p.`rep_date` AND t.`platform` = p.`platform` AND t.`zone` = p.`zone`
                ) AS t
                ORDER BY `rep_date`, `platform`, `zone`
            ;
        '''))

        query.run()
        query.get_results()
        is_success = YQLAdapter.is_success(query)
        if is_success:
            yt_adapter.leave_last_N_tables(source, N=3)
            with open('output.json', 'w') as f:
                json.dump({"table_path" : target}, f)
    else:
        with open('output.json', 'w') as f:
            json.dump({"table_path" : target}, f)


if __name__ == "__main__":
    main()
