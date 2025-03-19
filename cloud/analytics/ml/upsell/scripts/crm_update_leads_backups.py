import json
import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def main():
    backups_path = '//home/cloud_analytics/export/crm/update_call_center_leads/backups'
    yt_adapter = YTAdapter()
    yt_adapter.optimize_chunk_number('//home/cloud_analytics/export/crm/update_call_center_leads/update_leads')

    yql_adapter = YQLAdapter()
    query = yql_adapter.execute_query(dedent(f'''
    $date_time = DateTime::Format('%Y-%m-%dT%H:%M:00')(CurrentTzDateTime('Europe/Moscow'));
    $backup_path = '{backups_path}/update_leads_at_' || $date_time;

    INSERT INTO $backup_path
        SELECT *
        FROM `//home/cloud_analytics/export/crm/update_call_center_leads/update_leads`
    ;
    '''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w') as f:
            json.dump({"history_path" : '//home/cloud_analytics/ml/upsell/advanced_onboarding/adv_onb_history'}, f)
    yt_adapter.leave_last_N_tables(backups_path, 30)


if __name__ == "__main__":
    main()
