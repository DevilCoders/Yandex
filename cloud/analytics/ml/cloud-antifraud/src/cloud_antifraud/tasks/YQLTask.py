import itertools
import pickle
from os.path import join
import luigi
import math
import numpy as np
import pandas as pd
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.utils.path import childfile2str
from textwrap import dedent
from clan_tools import utils
from os.path import join
import logging

logger = logging.getLogger(__name__)

class YQLTask(luigi.Task):
    sql_file_path = luigi.Parameter()

    

    def run(self):
        query = childfile2str(__file__, self.sql_file_path)
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
        return luigi.LocalTarget(join('data', 'cache', self.sql_file_path))

      
