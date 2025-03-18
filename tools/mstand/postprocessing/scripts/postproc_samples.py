import logging
import os

from experiment_pool import Pool


class PoolPostprocessSample(object):
    @staticmethod
    def process_pool(pool_api):
        """
        :type pool_api: PoolForPostprocessAPI
        :rtype: pool
        """
        out_file = os.path.join(pool_api.dest_dir, "sample-out.txt")
        with open(out_file, "w") as fd:
            fd.write("hello world\n")

        # take first observation from source pool
        one_obs = pool_api.pool.observations[0]
        res_pool = Pool(observations=[one_obs], extra_data={"some extra data": "can go here"})

        # replace output pool
        pool_api.pool = res_pool

        # this sample does nothing with pool, just creates single file
        return pool_api


class ObservationPostprocessSample(object):
    @staticmethod
    def process_observation(obs_api):
        """
        :type obs_api: ObservationForPostprocessAPI
        :rtype:
        """

        file_name = "custom_sample_in_{}_metric_{}.txt".format(obs_api.observation.unique_str,
                                                               obs_api.metric_key.pretty_name)
        file_path = obs_api.add_custom_file(file_name)
        with open(file_path, "w") as custom_fd:
            custom_fd.write("sample_of_custom_data")

        logging.info("observation = %s, metric = %s", obs_api.observation, obs_api.metric_key)
        logging.info("calculating control")

        control_sum = 0.0
        control_count = 0
        for row in obs_api.control:
            obs_api.control.write_row(row)
            control_sum += sum(row)
            control_count += len(row)

        control_avg = float(control_sum) / control_count if control_count else 0.0

        logging.info("control avg: %s", control_avg)

        # observation extra_data
        obs_api.observation.extra_data = {"test": "observation extra data"}

        for index, exp in enumerate(obs_api.experiments):
            logging.info("calculating experiment %d", index)

            # metric result extra_data
            exp.extra_data = {"test": "metric_result extra data"}

            # experiment extra_data
            exp.experiment.extra_data = {"test": "experiment extra data"}

            for row in exp:
                values = [x - control_avg for x in row]
                exp.write_row(values)


class ExperimentPostprocessSample(object):
    @staticmethod
    def name():
        return "SampleExperimentPostprocessor"

    @staticmethod
    def process_experiment(exp_api):
        """
        :type exp_api: ExperimentForPostprocessAPI
        :rtype:
        """

        file_name = "custom_sample_in_{}_exp_{}_metric_{}.txt".format(exp_api.observation.unique_str,
                                                                      exp_api.experiment.testid,
                                                                      exp_api.metric_key.pretty_name)
        file_path = exp_api.add_custom_file(file_name)
        with open(file_path, "w") as custom_fd:
            custom_fd.write("sample_of_custom_data_in_experiment")

        exp_api.extra_data = {"hello": "world"}

        for index, row in enumerate(exp_api):
            new_row = [x * 3.1415926 for x in row]
            exp_api.write_row(new_row)


class ExperimentPostprocessSample1(object):
    @staticmethod
    def process_experiment(exp_api):
        exp_api.write_row([1.23])


class OfflineObservationPostprocessSample(object):
    @staticmethod
    def process_observation(obs_api):
        """
        :type obs_api: ObservationForPostprocessAPI
        :rtype:
        """

        # subtract control value for each of queries

        control_metric_vals = {}
        for row in obs_api.control:
            query_id = row[0]
            control_value = row[1]
            obs_api.control.write_row(row)
            control_metric_vals[query_id] = control_value

        for index, exp in enumerate(obs_api.experiments):
            logging.info("calculating experiment %d", index)

            # metric result extra_data
            exp.extra_data = {"test": "metric_result extra data"}

            # experiment extra_data
            exp.experiment.extra_data = {"test": "experiment extra data"}

            for row in exp:
                query_id = row[0]
                exp_value = row[1]
                control_value = control_metric_vals[query_id]
                values = [exp_value - control_value]
                exp.write_row(values)
