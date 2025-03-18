class EchoPostprocessor(object):
    # noinspection PyMethodMayBeStatic
    def process_experiment(self, exp_api):
        for row in exp_api:
            exp_api.write_row(row)


class ObsEchoPostprocessor(object):
    # noinspection PyMethodMayBeStatic
    def process_observation(self, obs_api):
        for row in obs_api.control:
            obs_api.control.write_row(row)

        for index, exp_api in enumerate(obs_api.experiments):
            for row in exp_api:
                exp_api.write_row(row)
