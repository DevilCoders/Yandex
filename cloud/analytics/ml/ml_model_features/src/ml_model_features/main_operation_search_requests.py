import logging.config
from textwrap import dedent
from datetime import timedelta, datetime
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def tokenize_requests(puids_path: str, result_path: str, dates_list: list):
    yql_adapter = YQLAdapter()

    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    DECLARE $dates AS List<String>;

    DEFINE ACTION $get_data_for_one_date($date) as
            $base_folder = "//statbox/cube/data/request/" || $date;
            $output = "{result_path}" || "/" || SUBSTRING($date, 0, 7);
            $puids_path = "{puids_path}";

            $puids = (
            SELECT
                distinct passport_uid
                FROM $puids_path
            );

            $source = (
            SELECT
                puid as passport_uid,
                canonized_query,
                normal_query,
                SUBSTRING(`datetime`, 0, 10) as query_date
            FROM $base_folder
            );


            $all_requests = (
            SELECT
                *
            FROM $source as search
            INNER JOIN $puids as choice
            on search.passport_uid == choice.passport_uid
            WHERE search.passport_uid IS NOT NULL
            );

            $raw_requests = (
            SELECT
                passport_uid,
                query_date,
                String::JoinFromList(CAST(AGGREGATE_LIST(canonized_query, 2) AS List<String>), " ") as requests
            FROM $all_requests
            GROUP BY passport_uid, query_date
            );

            $tokenizer = TextProcessing::MakeTokenizer(
                True as Lowercasing,
                True as Lemmatizing,
                "BySense" as SeparatorType,
                AsList("Word", "Number") as TokenTypes
            );

            INSERT INTO $output
            SELECT
                passport_uid,
                query_date,
                ListConcat(ListMap($tokenizer(requests), ($x) -> {{ RETURN GatherMembers(ChooseMembers($x,["Token"]))[0].1; }}), " ") AS tokenized
            FROM $raw_requests;
    END DEFINE;

    EVALUATE FOR $date IN $dates
        DO $get_data_for_one_date($date)
    '''))

    parameters = {
    '$dates': ValueBuilder.make_list([ValueBuilder.make_string(dt) for dt in dates_list]),
    }
    query.run(parameters=ValueBuilder.build_json_map(parameters))
    query.get_results()

if __name__ == "__main__":
    puids_path = "//home/cloud_analytics/ml/ml_model_features/raw/all_puids"
    result_path = "//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests"
    start_date = datetime.strptime('2021-01-01', '%Y-%m-%d').date()
    end_date = datetime.strptime('2022-03-01', '%Y-%m-%d').date()

    delta = end_date - start_date
    dates_list = [str(start_date + timedelta(days=i)) for i in range(1, delta.days + 1)]
    tokenize_requests(puids_path, result_path, dates_list)
