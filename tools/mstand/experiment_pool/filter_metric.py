import yaqutils.misc_helpers as umisc


class MetricFilter(object):
    def __init__(self,
                 metric_whitelist=None,
                 metric_blacklist=None,
                 ):
        self.metric_whitelist = umisc.stripped_set(metric_whitelist)
        self.metric_blacklist = umisc.stripped_set(metric_blacklist)

    @staticmethod
    def from_cli_args(cli_args):
        return MetricFilter(
            metric_whitelist=cli_args.metric_names,
            metric_blacklist=cli_args.remove_metric_names,
        )

    @staticmethod
    def add_cli_args(parser):
        """
        :type parser: argparse.ArgumentParser
        """
        parser.add_argument(
            "--metric-names",
            nargs="+",
            help="only accept metrics with these names",
            default=[],
        )
        parser.add_argument(
            "--remove-metric-names",
            nargs="+",
            help="remove metrics with these names",
            default=[],
        )

    def filter_metric_for_pool(self, pool):
        """
        :type pool: experiment_pool.Pool
        """
        for observation in pool.observations:
            self.filter_metric_for_observation(observation)

    def filter_metric_for_observation(self, observation):
        """
        :type observation: experiment_pool.Observation
        """
        self.filter_metric_for_experiment(observation.control)
        for experiment in observation.experiments:
            self.filter_metric_for_experiment(experiment)

    def filter_metric_for_experiment(self, experiment):
        """
        :type experiment: experiment_pool.Experiment
        """
        new_metric_results = [mr for mr in experiment.metric_results if self.accept_metric_result(mr)]
        experiment.clear_metric_results()
        experiment.add_metric_results(new_metric_results)

    def accept_metric_result(self, metric_result):
        """
        :type metric_result: experiment_pool.MetricResult
        """
        pretty_name = metric_result.metric_key.pretty_name().strip()
        name = metric_result.metric_key.name.strip()

        if pretty_name in self.metric_blacklist or name in self.metric_blacklist:
            return False
        if self.metric_whitelist and pretty_name not in self.metric_whitelist and name not in self.metric_whitelist:
            return False
        return True
