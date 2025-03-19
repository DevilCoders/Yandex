
import luigi
import logging.config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.conf import read_conf
from cloud_antifraud.tasks.AnalyzeFraud import AnalyzeFraud
from cloud_antifraud.data_adapters.FraudFeaturesAdapter import FraudFeaturesAdapter
from cloud_antifraud.feature_extraction.FeatureExtractor import FeatureExtractor
from cloud_antifraud.ml.MLProblem import MLProblem
import luigi.tools.deps_tree as deps_tree
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
import json
from cloud_antifraud.ml.FraudAnalyzer import FraudAnalyzer
import logging.config



logging.config.dictConfig(read_conf('config/logger.yml'))
luigi.configuration.add_config_path('config/luigi.conf')


if __name__ == '__main__':
    task = AnalyzeFraud()
    print(deps_tree.print_tree(task))
    luigi.build([task], local_scheduler=True)

    with open('output.json', 'w') as f:
        json.dump({'status': 'success'}, f)

   