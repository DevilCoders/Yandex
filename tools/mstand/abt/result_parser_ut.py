import abt.result_parser
import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import MetricColoring
from experiment_pool import Observation


# noinspection PyClassHasNoInit
class TestResultParser:
    def test_parse(self):
        summary = {
            "abt_metric_name": [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.18, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.10, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.12, 'diffprec1': 0.11, 'diffprec05': 0.11, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 94.11, 'pvalue': 0.19, 'prec': 0.1, 'ttest': 93.1, 'obf7': 0.0, 'waldtest': 89.1,
                    'delta_val': 0.11, 'delta_prec': 0.1
                },
                {
                    'diffprec01': 0.1, 'diffprec1': 0.1, 'diffprec05': 0.1, 'val': 2.16, 'obf14': 99.9, 'mwtest': 100.0,
                    'pvalue': 0.0, 'prec': 0.1, 'ttest': 100.0, 'obf7': 99.9, 'waldtest': 100.0, 'delta_val': 0.1,
                    'delta_prec': 0.1
                }
            ]
        }

        daily = {'abt_metric_name': [
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.125, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.181, 'diffprec1': 0.115, 'diffprec05': 0.14, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 63.12, 'prec': 0.1139, 'obf7': 0.0, 'waldtest': 89.17, 'ttest': 68.15
                },
                {
                    'diffprec01': 0.19, 'diffprec1': 0.192, 'diffprec05': 0.119, 'val': 2.1, 'obf14': 99.9,
                    'mwtest': 99.14, 'prec': 0.175, 'obf7': 99.9, 'waldtest': 100.0, 'ttest': 99.16
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.1269, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.129, 'diffprec1': 0.102, 'diffprec05': 0.182, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 56.12, 'prec': 0.17, 'obf7': 0.0, 'waldtest': 79.12, 'ttest': 61.11
                },
                {
                    'diffprec01': 0.193, 'diffprec1': 0.112, 'diffprec05': 0.15, 'val': 2.1, 'obf14': 99.9,
                    'mwtest': 99.15, 'prec': 0.121, 'obf7': 99.9, 'waldtest': 100.0, 'ttest': 99.17
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.124, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.134, 'diffprec1': 0.159, 'diffprec05': 0.102, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 95.15, 'prec': 0.1851, 'obf7': 0.0, 'waldtest': 75.14, 'ttest': 93.1
                },
                {
                    'diffprec01': 0.173, 'diffprec1': 0.128, 'diffprec05': 0.11, 'val': 2.1, 'obf14': 99.5,
                    'mwtest': 99.19, 'prec': 0.133, 'obf7': 99.9, 'waldtest': 99.17, 'ttest': 99.1
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.1461, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.155, 'diffprec1': 0.132, 'diffprec05': 0.193, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 82.15, 'prec': 0.192, 'obf7': 0.0, 'waldtest': 87.1, 'ttest': 87.12
                },
                {
                    'diffprec01': 0.163, 'diffprec1': 0.128, 'diffprec05': 0.117, 'val': 2.1, 'obf14': 99.9,
                    'mwtest': 99.18, 'prec': 0.1926, 'obf7': 99.9, 'waldtest': 100.0, 'ttest': 100.0
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.17, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0},
                {
                    'diffprec01': 0.116, 'diffprec1': 0.151, 'diffprec05': 0.192, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 15.17, 'prec': 0.1688, 'obf7': 0.0, 'waldtest': 10.17, 'ttest': 35.15
                },
                {
                    'diffprec01': 0.177, 'diffprec1': 0.195, 'diffprec05': 0.11, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 99.17, 'prec': 0.149, 'obf7': 0.0, 'waldtest': 99.12, 'ttest': 99.13
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.1512, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.13, 'diffprec1': 0.14, 'diffprec05': 0.142, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 21.13, 'prec': 0.1177, 'obf7': 0.0, 'waldtest': 89.17, 'ttest': 5.1
                },
                {
                    'diffprec01': 0.139, 'diffprec1': 0.189, 'diffprec05': 0.101, 'val': 2.1, 'obf14': 99.9,
                    'mwtest': 99.15, 'prec': 0.1541, 'obf7': 99.9, 'waldtest': 100.0, 'ttest': 99.12
                }
            ],
            [
                {
                    'diffprec01': 0.0, 'diffprec1': 0.0, 'diffprec05': 0.0, 'val': 2.1, 'obf14': 50.0, 'mwtest': 0.0,
                    'prec': 0.139, 'obf7': 50.0, 'waldtest': 50.0, 'ttest': 0.0
                },
                {
                    'diffprec01': 0.16, 'diffprec1': 0.149, 'diffprec05': 0.197, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 69.15, 'prec': 0.141, 'obf7': 0.0, 'waldtest': 42.14, 'ttest': 75.16
                },
                {
                    'diffprec01': 0.117, 'diffprec1': 0.124, 'diffprec05': 0.187, 'val': 2.1, 'obf14': 0.0,
                    'mwtest': 99.19, 'prec': 0.12, 'obf7': 99.0, 'waldtest': 99.19, 'ttest': 99.14
                }
            ]
        ]
        }
        dates = utime.DateRange(utime.parse_date_msk("20160530"), utime.parse_date_msk("20160603"))
        control = Experiment(testid="1111")
        exp1 = Experiment(testid="2222")
        exp2 = Experiment(testid="3333")
        observation = Observation(obs_id="100500", dates=dates, control=control, experiments=[exp1, exp2])
        parser = abt.result_parser.ResultParser(detailed=True)

        testids_info = [control.testid, exp1.testid, exp2.testid]
        parser.parse(observation=observation, testids=testids_info, summary=summary, daily=daily,
                     coloring=MetricColoring.NONE)
