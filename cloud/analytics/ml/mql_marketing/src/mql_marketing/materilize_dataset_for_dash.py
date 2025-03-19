import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def prepare_mal_table(path_mal_folder, path_puids_folder, path_mal_list):

    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()
    available_dates = yt_adapter.yt.list(path_puids_folder)
    existing_dates = yt_adapter.yt.list(path_mal_folder)
    N_num = len(available_dates)

    dates_to_process = sorted(set(available_dates)-set(existing_dates))

    for curr_date in dates_to_process:
        logger.info(f'Calculations as on {curr_date}')
        query = yql_adapter.execute_query(dedent(f'''
        $source_table = '{path_puids_folder}/{curr_date}';
        $result_table = '{path_mal_folder}/{curr_date}';
        $mal_table = '{path_mal_list}';

        $mal_table =
            SELECT
                String::AsciiToLower(Unicode::ToLower(`acc_name`)) AS `acc_name_t`,
                ListConcat(AGG_LIST(Cast(`ba_id` AS String)), "; ") AS `ba_id`,
                ListConcat(AGG_LIST(Cast(`inn` AS String)), "; ") AS `mal_inn`,
                ListConcat(AGG_LIST(Cast(`kpp` AS String)), "; ") AS `mal_kpp`
            FROM $mal_table WHERE `acc_name` IS NOT NULL AND `acc_name` != '' GROUP BY `acc_name`
        ;

        INSERT INTO $result_table WITH TRUNCATE
            SELECT * FROM $source_table AS st
            LEFT JOIN $mal_table AS mt ON st.`mal_name` = mt.`acc_name_t`
            WHERE `mal_name` != ''
                AND `mal_name` IS NOT NULL
        ;'''))

        YQLAdapter.attach_files(utils.__file__, 'yql', query)
        query.run()
        query.get_results()
        is_success = YQLAdapter.is_success(query)
        if not is_success:
            raise RuntimeError('YQL script is failed')

        logger.info('Starting table optimization...')
        yt_adapter.optimize_chunk_number(f'{path_mal_folder}/{curr_date}')
    yt_adapter.leave_last_N_tables(path_mal_folder, N_num)


@timing
def prepare_materilized_table(path_mal_folder: str, path_puids_folder: str, path_result_table: str):
    logger.info('Starting query execution...')

    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()
    path_mal_table = yt_adapter.last_table_name(path_mal_folder)
    path_puids_table = yt_adapter.last_table_name(path_puids_folder)

    query = yql_adapter.execute_query(dedent(f'''
    $res_table = '{path_result_table}';
    $yson_to_list = ($col) -> {{
        RETURN Yson::ConvertTo($col, List<Utf8>) ?? []
    }};

    $clear_list = ($col) -> {{
        $col_wo_plus = ListMap($col, ($x) -> {{ RETURN Re2::Replace("^\\\\+?")($x, ""); }});
        $col_with_plus = ListMap($col_wo_plus, ($x) -> {{ RETURN "+" || $x; }});
        $col_wo_null = ListNotNull($col_with_plus);
        $col_distinct = ListUniq($col_wo_null);
        RETURN $col_distinct
    }};

    $mal_normal_list =
        SELECT
            `mal_name` AS `MAL_company_name`,
            `company_name` AS `Company_name`,
            $clear_list(ListExtend($yson_to_list(`iam_phone`), [Cast(`sprav_phone` AS Utf8)])) AS `Phones`,
            $yson_to_list(`iam_email`)[0] AS `Email`,
            `puid` AS `Puid`,
            `ba_id` AS `Billing_account_id`,
            Coalesce($yson_to_list(`blng_name`)[0], `sprav_fio`) AS `Fio`
        FROM `{path_mal_table}`
        WHERE 1=1
            AND ListLength($clear_list(ListExtend($yson_to_list(`iam_phone`), [Cast(`sprav_phone` AS Utf8)]))) > 0
            AND ListLength($yson_to_list(`iam_email`)) > 0
            AND `ba_id` IS NULL
    ;

    $by_puids_score =
        SELECT
            t.puid AS `puid`,
            Coalesce(`visit_count_more2hits`, 0) AS `visit_count_more2hits`,
            Math::Round(Coalesce(`visit_hits_avg_more2hits`, 0), -2) AS `visit_hits_avg_more2hits`,
            Coalesce(`events_reg_count`, 0) AS `events_reg_count`,
            Coalesce(`forms_submitted`, 0) AS `forms_submitted`,
            Coalesce(`docs_loaded`, 0) AS `docs_loaded`,
            Coalesce(`visited_from_letter`, 0) AS `visited_from_letter`,
            Coalesce(`subscribed_acc_utm`, 0) AS `subscribed_acc_utm`,
            If(Coalesce(`iam_num_created_clouds`, 0) > 0, 1, 0) AS `has_cloud`,
            Math::Round(Cast(
                (Cast(Least(Coalesce(`visit_count_more2hits`, 0), 9) AS Int32) / 3)*2 +
                Least(Coalesce(`events_reg_count`, 0), 2)*5 +
                Least(Coalesce(`forms_submitted`, 0), 2)*5 +
                Least(Coalesce(`docs_loaded`, 0), 2)*3 +
                Least(Coalesce(`visited_from_letter`, 0), 2)*2 +
                Least(Coalesce(`subscribed_acc_utm`, 0), 1)*5 +
                If(Coalesce(`iam_num_created_clouds`, 0) > 0, 1, 0)*8
            AS Double), 0) AS `ind_score`
        FROM `{path_puids_table}` AS t
        LEFT JOIN `//home/cloud_analytics/ml/mql_marketing/result/puids_with_cons` AS c ON t.puid = c.puid
        WHERE Coalesce(`company_from_sprav`, `company_from_events`) IS NOT NULL
    ;

    $mal_list_score =
        SELECT
            `mal_name`,
            Count( DISTINCT `puid`) AS `num_related_puids`,
            Math::Round(Cast(
                Sum((Cast(Least(Coalesce(`visit_count_more2hits`, 0), 9) AS Int32) / 3)*2) +
                Sum(Least(Coalesce(`events_reg_count`, 0), 2)*5) +
                Sum(Least(Coalesce(`forms_submitted`, 0), 2)*5) +
                Sum(Least(Coalesce(`docs_loaded`, 0), 2)*3) +
                Sum(Least(Coalesce(`visited_from_letter`, 0), 2)*2) +
                Sum(Least(Coalesce(`subscribed_acc_utm`, 0), 1)*5) +
                Sum(IF(`iam_num_created_clouds`>0, 1, 0))*8
            AS Double), 0) AS `score`
        FROM `{path_mal_table}` AS t
        WHERE `ba_id` IS NULL
        GROUP BY `mal_name`
    ;

    INSERT INTO $res_table WITH TRUNCATE
    SELECT
        `MAL_company_name`,
        `num_related_puids` AS `Puids_cnt`,
        `score` AS `MAL_company_score`,
        `Company_name`,
        `Puid`,
        `Fio`,
        ListConcat(`Phones`, '; ') AS `Phones`,
        `Email`,
        `ind_score` AS `Puid_score`,
        `visit_count_more2hits`,
        `visit_hits_avg_more2hits`,
        `events_reg_count`,
        `forms_submitted`,
        `docs_loaded`,
        `visited_from_letter`,
        `subscribed_acc_utm`,
        `has_cloud`
    FROM $mal_normal_list AS mnl
    LEFT JOIN $by_puids_score AS bps ON mnl.`Puid` = bps.`puid`
    LEFT JOIN $mal_list_score AS mls ON mnl.`MAL_company_name` = mls.`mal_name`
    ORDER BY `Puid_score` DESC
    ;'''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if not is_success:
        raise RuntimeError('YQL script is failed')

    logger.info('Starting table optimization...')
    yt_adapter.optimize_chunk_number(path_result_table)
