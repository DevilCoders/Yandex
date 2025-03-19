from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools import utils
import os
from textwrap import dedent
from clan_tools.utils.time import curr_utc_date_iso
from yql.client.operation import YqlSqlOperationRequest

class EmailsDataAdapter:
    def __init__(self, yql_adapter:YQLAdapter, result_table:str, append=True):
        self._yql_adapter = yql_adapter
        self._result_table = result_table
        self._append = append
    
    
    
    def collect_data(self, date:str=None):
        if date is None:
            date = curr_utc_date_iso(days_lag=1) 

        def _min(date):
            return f", '{date}'" if self._append else ''  
        
        def _historical():
            return '''
                    SELECT * 
                    FROM $result_table
                    UNION ALL
                    ''' \
                    if self._append else ''


        
        collect_sql = dedent(f'''
            PRAGMA yt.Pool='cloud_analytics_pool';
            PRAGMA Library("sender_delivery.sql");
            PRAGMA Library("sender_clicks.sql");
            PRAGMA Library("notify_delivery.sql");
            PRAGMA Library("console_clicks.sql");
            
            PRAGMA yson.DisableStrict;



            IMPORT sender_delivery SYMBOLS $sender_delivery;
            IMPORT sender_clicks SYMBOLS $sender_clicks;
            IMPORT notify_delivery SYMBOLS $notify_delivery;
            IMPORT console_clicks SYMBOLS $console_clicks;

            $result_table = "{self._result_table}";

            DEFINE SUBQUERY $sender_delivery_table() AS 
                SELECT * FROM RANGE('//logs/sendr-delivery-log/1d'{_min(date)})
            END DEFINE;

            DEFINE SUBQUERY $sender_clicks_table() AS 
                SELECT * FROM RANGE('//logs/sendr-click-log/1d'{_min(date)})
            END DEFINE;

            DEFINE SUBQUERY $notify_delivery_table() AS 
                SELECT * FROM RANGE('//home/logfeller/logs/qloud-runtime-log/1d'{_min(date)})
            END DEFINE;

            DEFINE SUBQUERY $console_clicks_table() AS 
                SELECT * FROM RANGE('//home/logfeller/logs/dataui-prod-nodejs-log/1d'{_min(date)})
            END DEFINE;

            $select_sender_delivery = (SELECT *
                                       FROM $sender_delivery($sender_delivery_table));

            $select_sender_clicks = ( SELECT sc.event AS event, 
                                             sc.letter_id AS letter_id, 
                                             sc.link_url AS link_url, 
                                             sc.message_id AS message_id, 
                                             sc.source AS source, 
                                             sc.status AS status, 
                                             sc.unixtime AS unixtime,
                                             NVL(sc.campaign_id, sd.campaign_id) AS campaign_id, 
                                             NVL(sc.campaign_type, sd.campaign_type) AS campaign_type, 
                                             NVL(sc.channel, sd.channel) AS channel, 
                                             NVL(sc.email, sd.email) AS email,
                                             NVL(sc.letter_code, sd.letter_code) AS letter_code, 
                                             NVL(sc.tags, sd.tags) AS tags, 
                                             NVL(sc.title, sd.title) AS title, 
                                             NVL(sc.message_type, sd.message_type) AS message_type
                                      FROM $sender_clicks($sender_clicks_table) AS sc
                                      INNER JOIN $select_sender_delivery  AS sd
                                        ON sc.message_id = sd.message_id
                                     );

            $select_notify_delivery = (SELECT *
                                       FROM $notify_delivery($notify_delivery_table));

            $select_console_clicks = ( SELECT sc.event AS event, 
                                              sc.letter_id AS letter_id, 
                                              sc.link_url AS link_url, 
                                              sc.message_id AS message_id, 
                                              sc.source AS source, 
                                              sc.status AS status, 
                                              sc.unixtime AS unixtime,
                                              NVL(sc.campaign_id, sd.campaign_id) AS campaign_id, 
                                              NVL(sc.campaign_type, sd.campaign_type) AS campaign_type, 
                                              NVL(sc.channel, sd.channel) AS channel, 
                                              NVL(sc.email, sd.email) AS email,
                                              NVL(sc.letter_code, sd.letter_code) AS letter_code, 
                                              NVL(sc.tags, sd.tags) AS tags, 
                                              NVL(sc.title, sd.title) AS title, 
                                              NVL(sc.message_type, sd.message_type) AS message_type
                                      FROM $console_clicks($console_clicks_table) AS sc
                                      INNER JOIN $select_notify_delivery  AS sd
                                        ON sc.message_id = sd.message_id
                                     );


            $union = (
                SELECT DISTINCT *
                FROM 
                (
                    {_historical()}

                    SELECT * 
                    FROM $select_sender_delivery 

                    UNION ALL
                    
                    SELECT *
                    FROM $select_sender_clicks

                    UNION ALL

                    SELECT *
                    FROM $select_notify_delivery

                    UNION ALL

                    SELECT *
                    FROM $select_console_clicks
                ));




            INSERT INTO $result_table WITH TRUNCATE 
            SELECT * FROM $union
        ''')
        
        req : YqlSqlOperationRequest = self._yql_adapter.execute_query(collect_sql, to_pandas=False)
        req = YQLAdapter.attach_files(__file__, 'sql', req)
        req = YQLAdapter.attach_files(__file__, 'reduce.py', req)
        req.run()
        query_res = req.get_results()
        return req