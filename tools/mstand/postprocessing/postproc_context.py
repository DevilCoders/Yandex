import os
import random

import postprocessing.postproc_helpers as pp_helpers
import yaqutils.file_helpers as ufile
from experiment_pool import Experiment  # noqa
from experiment_pool import Observation  # noqa


class PostprocGlobalContext(object):
    def __init__(self, pp_container, source_dir, dest_dir, preserve_original_names=False):
        """
        :type pp_container: user_plugins.PluginContainer
        :type source_dir: str
        :type dest_dir: str
        :type preserve_original_names: bool
        """
        self.postproc_container = pp_container
        self.source_dir = source_dir
        self.dest_dir = dest_dir
        self.custom_dir_name = "custom_files"
        # union of all custom files in all observations
        self.custom_files = set()

        # set in postproc_engine.postprocess_results_core
        self.pool_extra_data = None

        # MSTAND-1373
        self.preserve_original_names = preserve_original_names


class PostprocObservationContext(object):
    def __init__(self, global_ctx, observation, unique_str, pp_instance, pp_key):
        """
        :type global_ctx: PostprocGlobalContext
        :type observation: Observation
        :type unique_str: str
        :type pp_instance:
        :type pp_key: user_plugins.PluginKey
        """
        self.pp_instance = pp_instance
        self.pp_key = pp_key
        self.global_ctx = global_ctx
        self.observation = observation
        self.random_seed = random.random()
        self.custom_files = set()
        self.unique_str = unique_str

    def add_custom_file(self, file_name):
        if not pp_helpers.is_valid_custom_file_name(file_name):
            raise Exception("File name '{}' is invalid".format(file_name))

        if file_name in self.custom_files:
            raise Exception("Custom file name {} already exists".format(file_name))

        self.custom_files.add(file_name)

        dest_dir = self.global_ctx.dest_dir
        custom_dir_name = self.global_ctx.custom_dir_name
        custom_dir_path = os.path.join(dest_dir, custom_dir_name)

        ufile.make_dirs(custom_dir_path)
        return os.path.join(custom_dir_path, file_name)

    @property
    def postprocessor(self):
        return self.pp_instance


class PostprocExperimentContext(object):
    def __init__(self, observation_ctx, experiment, unique_str):
        """
        :type observation_ctx: PostprocObservationContext
        :type experiment: Experiment
        :type unique_str: str
        """
        self.observation_ctx = observation_ctx
        self.experiment = experiment
        self.unique_str = observation_ctx.unique_str + "_" + unique_str

    @property
    def postprocessor(self):
        return self.observation_ctx.pp_instance
