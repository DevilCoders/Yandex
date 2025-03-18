import logging


class ControlExpRatio(object):
    @staticmethod
    def process_observation(obs_api):
        """
        :type obs_api: ObservationForPostprocessAPI
        :rtype:
        """

        control_value = obs_api.control.significant_metric_value()
        if abs(control_value) < 1e-8:
            logging.info("Obs %s, metric %s: control value is near zero, cannot produce correct ratio. ",
                         obs_api.observation, obs_api.metric_key)
            control_value = None

        obs_api.control.write_row([None])

        for index, exp in enumerate(obs_api.experiments):
            # logging.info("calculating experiment %d: %s", index, exp)
            exp_value = exp.significant_metric_value()
            if control_value is not None:
                ratio = float(exp_value) / float(control_value)
                exp.write_row([ratio])
            else:
                exp.write_row([None])
