import itertools
import pickle
from os.path import join
import luigi
import math
import numpy as np
import pandas as pd
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from textwrap import dedent
from clan_tools import utils
from os.path import join
import logging

logger = logging.getLogger(__name__)

class BadAccountsHistory(luigi.Task):

    

    def run(self):
        query = dedent(f'''
                PRAGMA yt.Pool = 'cloud_analytics_pool';
                PRAGMA Library('tables.sql');
                PRAGMA Library('time.sql');
                IMPORT time SYMBOLS $date_time;
                IMPORT tables SYMBOLS $last_non_empty_table;

                $result_table = '//home/cloud_analytics/tmp/antifraud/bad_accounts_history';

                $bad_accounts = '//home/antifraud/export/cloud/bad_accounts/1h';
                --sql
                $rules_structs = (SELECT
                        AsStruct(`karma` as karma,
                                 $date_time(TableName()) as time,
                                 String::JoinFromList(CAST(recalc_rules AS List<String>), ',') as recalc_rules,
                                `passport_uid` as passport_uid,
                                `is_suspicious` as is_suspicious,
                                 String::JoinFromList(CAST(cloud_rules AS List<String>), ',') as cloud_rules,
                                 String::JoinFromList(CAST(created_rules AS List<String>), ',') as created_rules,
                                 String::JoinFromList(CAST(suspicious_rules AS List<String>), ',') as suspicious_rules,
                                `is_bad` as is_bad) as `rules`,
                        AsStruct({ ','.join([str(i) + " in cloud_rules AS cloud_rule" + str(i) for i in range(12)]) }) as cloud_rules,
                        AsStruct({ ','.join([str(i) + " in created_rules AS created_rule" + str(i) for i in range(8)]) }) as created_rules,
                        AsStruct(0 in suspicious_rules AS suspicious_rule0,
                                 1 in suspicious_rules AS suspicious_rule1) as susp_rules
                FROM  RANGE($bad_accounts));
                --endsql

                $flattened_rules = (select * 
                                    from $rules_structs
                                    flatten columns);

                insert into $result_table with truncate
                select * from $flattened_rules;
        ''')
        cons = YQLAdapter().execute_query(query, to_pandas=False)
        YQLAdapter.attach_files(__file__, 'sql', cons)
        YQLAdapter.attach_files(utils.__file__, 'yql', cons)
        cons.run()
        cons.get_results()
        is_success = YQLAdapter.is_success(cons)
        if is_success:
            with self.output().open('w') as f:
                f.write(cons.share_url) 
       

    def output(self):
        return luigi.LocalTarget(join('data', 'cache', 'sql', 'bad_accounts_history.sql'))

      
