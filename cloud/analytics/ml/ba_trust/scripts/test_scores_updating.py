import json
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def main() -> None:
    yql_adapter = YQLAdapter()

    df = yql_adapter.run_query_to_pandas(dedent('''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
    use hahn;

    SELECT Path,
    DateTime::ToHours(CurrentUtcTimestamp() - CAST(Yson::LookupString(Attributes, "modification_time") as Timestamp)) as `hours`
    FROM FOLDER("//home/cloud_analytics/ml/ba_trust", "modification_time")
    WHERE
        Type = "table"
        and String::EndsWith(Path, "/trust_scores")
    '''))

    logger.debug(f"hours: {df['hours'][0]}")
    assert(df['hours'][0] < 6)

    with open('output.json', 'w+') as f:
        json.dump({"output" : "//home/cloud_analytics/ml/ba_trust/trust_scores"}, f)

if __name__ == "__main__":
    main()





