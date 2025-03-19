
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter

class Postprocessor:
    def __init__(self, token_yql, folder) -> None:
        self._token_yql = token_yql
        self._folder = folder
    
    def combine_cube(self):
        path = '//home/{0}/cubes/emailing_events/emailing_events'.format(self._folder)
        query = dedent(f'''
            PRAGMA File('libcrypta_identifier_udf.so', 'yt://hahn/home/crypta/public/udfs/stable/libcrypta_identifier_udf.so');
            PRAGMA Udf('libcrypta_identifier_udf.so');
            PRAGMA yt.Pool='cloud_analytics_pool';

            $result_table = '{path}';

            --sql
            $acq = (
                SELECT DISTINCT
                    Identifiers::NormalizeEmail(user_settings_email) as email,
                    billing_account_id,
                    puid,
                    ba_state,
                    IF(block_reason IS NULL, 'unlocked', block_reason) as block_reason,
                    is_fraud,
                    ba_usage_status,
                    segment,
                    account_name,
                    event_time as cloud_created_time,
                    first_ba_created_datetime as ba_created_time,
                    first_first_trial_consumption_datetime as first_trial_consumption_time,
                    first_first_paid_consumption_datetime as first_paid_consumption_time
                FROM
                    `//home/cloud_analytics/cubes/acquisition_cube/cube`
                WHERE
                    event = 'cloud_created'
                    AND master_account_id = ''
                    AND billing_account_id != ''
            );

            --sql
            $emails = (
                SELECT
                        *
                FROM
                    `//home/{self._folder}/cubes/emailing_events/notify_events`
                UNION ALL
                SELECT
                    *
                FROM
                    `//home/{self._folder}/cubes/emailing_events/email_events`
                UNION ALL
                SELECT
                    *
                FROM
                    `//home/{self._folder}/cubes/emailing_events/add_to_nurture_stream`
            );

            --sql
            $emails_with_accounts = (
                SELECT
                    emails.*,
                    billing_account_id,
                    puid,
                    ba_state,
                    block_reason,
                    is_fraud,
                    ba_usage_status,
                    segment,
                    account_name,
                    cloud_created_time,
                    ba_created_time,
                    first_trial_consumption_time,
                    first_paid_consumption_time
                FROM $emails as emails
                LEFT JOIN $acq as acq
                ON emails.email = acq.email
            );

            --sql
            INSERT INTO $result_table WITH TRUNCATE
            SELECT *
            FROM $emails_with_accounts
            ORDER BY event_time;
            ''')
        
        req = YQLAdapter(token=self._token_yql).execute_query(query)
        req.run()
        req.get_results()
        return path


        
        