# -*- coding: utf-8 -*-
"""
This piece of code was taken from
arcadia/quality/ab_testing/scripts/exp_veles/exp_mr_server/scripts/calc_mstand_executable.py
during MSTAND-1518 and rewritten.
"""
import datetime
import json
import logging
import time
import urllib2

import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv

from mstand_enums.mstand_online_enums import ServiceEnum
from yaqutils.nirvana_api import NirvanaProxy


def check_mstand_services(services):
    return isinstance(services, list) and \
        all([service in ServiceEnum.SOURCES for service in ServiceEnum.convert_aliases(services)])


def write_graph_progress(progress):
    unirv.log_nirvana_progress_fraction("Graph execution", progress)


class ParameterBuilder(object):
    def __init__(self, allowed_params):
        self._allowed_params = allowed_params
        self._building_result = {}

    def __setitem__(self, name, value):
        if name not in self._allowed_params:
            logging.info("Parameter by name %s is not supported", name)
            return
        if value is not None:
            logging.info("Adding parameter value %s: %s", name, value)
            self._building_result[name] = value
        else:
            logging.info("Value 'None' is ignored for parameter %s", name)

    def __call__(self):
        return self._building_result


class Mode(object):
    project_code = None
    quota_project_id = None
    additional_settings = {}
    workflow_permissions = None
    workflow_description = None

    def __init__(self, params):
        self.params = params
        self.metric_params = params["metric_params"]

    def generate_workflow_name(self):
        raise NotImplementedError()

    def construct_supported_params(self, supported_global_params):
        raise NotImplementedError()

    def add_meta(self, raw_result, workflow=None):
        return raw_result


class MStandMode(Mode):
    project_code = "mstand_abt_launches"
    quota_project_id = "mstand-abt"
    workflow_description = "mstand calculation graph"

    def generate_workflow_name(self):
        return "Calc Mstand Metric {}-{}-{}-{}".format(
            self.params["obsid"],
            self.metric_params["config_name"],
            self.params["start"],
            self.params["finish"]
        )

    def construct_supported_params(self, supported_global_params):
        if not supported_global_params:
            return {}
        logging.info("Global parameters were received: %s", umisc.to_lines(supported_global_params))
        builder = ParameterBuilder(set(supported_global_params))
        builder["observation-id"] = self.params.get("obsid")
        builder["date-from"] = self.params.get("start")
        builder["date-to"] = self.params.get("finish")
        builder["yt_pool"] = self.metric_params.get("yt_pool")
        builder["yt_token"] = self.metric_params.get("yt_token")
        builder["ab-task-id"] = self.params.get("task")

        for option, value in self.additional_settings.iteritems():
            builder[option] = value

        metric_options = self.metric_params.get("options") or {}
        for option, value in metric_options.iteritems():
            if option == "services" and check_mstand_services(value):
                value = sorted(ServiceEnum.convert_aliases(value))
            builder[option] = value

        result = builder()
        logging.info("Global parameters values were collected: %s", umisc.to_lines(result))
        return result

    def add_meta(self, mstand_pool, workflow=None):
        logging.info("Adding meta data to result...")
        config_named_group = None
        grouped_metrics = {}
        if mstand_pool.get("meta") is not None and mstand_pool["meta"].get("groups") is not None:
            for group in mstand_pool["meta"]["groups"]:
                group["name"] = group["key"]
                group["url"] = self.metric_params.get("wiki_url") or ""
                for metric in group["metrics"]:
                    grouped_metrics[metric["name"]] = group["name"]
                    if metric.get("elite") is None or not (self.metric_params.get("elite") or False):
                        metric["elite"] = False
                    if metric.get("releable") is None or not (self.metric_params.get("releable") or False):
                        metric["releable"] = False
                    if metric.get("controls_validated") is None or not (self.metric_params.get("validate") or False):
                        metric["controls_validated"] = False
                if group["name"] == self.metric_params["config_name"]:
                    config_named_group = group
                    mstand_pool["meta"]["groups"].remove(group)
        metrics_meta = []
        for metric_key in mstand_pool["data"]["metrics"]:
            if metric_key not in grouped_metrics:
                metrics_meta.append({
                    "name": metric_key,
                    "elite": self.metric_params.get("elite") or False,
                    "releable": self.metric_params.get("releable") or False,
                    "controls_validated": self.metric_params.get("validate") or False
                })

        if "meta" not in mstand_pool:
            mstand_pool["meta"] = {}
        if mstand_pool["meta"].get("groups") is None:
            mstand_pool["meta"]["groups"] = []
        if config_named_group is not None:
            config_named_group["metrics"].extend(metrics_meta)
        elif metrics_meta:
            config_named_group = {
                "metrics": metrics_meta,
                "name": self.metric_params["config_name"],
                "key": self.metric_params["config_name"],
                "url": self.metric_params.get("wiki_url") or "",
            }
        if config_named_group:
            mstand_pool["meta"]["groups"] = [config_named_group] + mstand_pool["meta"]["groups"]
        if workflow is not None:
            mstand_pool["meta"]["workflow_url"] = workflow

        logging.info("Meta data was successfully added to result")
        return mstand_pool


ALLOWED_RUNNER_MODES = {"mstand": MStandMode}


class WorkflowRunnerSetupError(Exception):
    pass


class WorkflowCreationError(Exception):
    pass


class SetGlobalParametersError(Exception):
    pass


class WorkflowValidationError(Exception):
    pass


class WorkflowStartingError(Exception):
    pass


class WorkflowExecutionError(Exception):
    pass


class WorkflowResultDownloadingError(Exception):
    pass


class WorkflowRunner(object):
    def __init__(self, token, workflow_id, instance_id, params, mode, calc_timeout):
        self.proxy = NirvanaProxy(token)
        self.base_workflow = workflow_id
        self.base_instance = instance_id
        self.mode = self.get_mode(mode, params)
        self.timeout = calc_timeout  # timeout is in seconds

        self._workflow_id = None
        self._instance_id = None

    @property
    def workflow_id(self):
        return self._workflow_id

    @property
    def instance_id(self):
        return self._instance_id

    def get_workflow_url(self):
        return unirv.make_nirvana_workflow_url_by_id(self.workflow_id, self.instance_id)

    def get_mode(self, mode, params):
        if mode not in ALLOWED_RUNNER_MODES:
            raise WorkflowRunnerSetupError("mode {} is not supported".format(mode))
        return ALLOWED_RUNNER_MODES[mode](params)

    def get_global_params(self):
        supported_global_params = self.proxy.get_global_parameters(self.workflow_id, self.instance_id)
        if not supported_global_params:
            logging.warning("Workflow %s lacks global parameters", self.get_workflow_url())
            return {}
        return supported_global_params

    def create_new_workflow(self):
        self._workflow_id = None
        self._instance_id = None
        logging.info("Creating new workflow...")
        try:
            workflow_name = self.mode.generate_workflow_name()
            self._workflow_id = self.proxy.clone_workflow(src_workflow_id=self.base_workflow,
                                                          workflow_instance_id=self.base_instance,
                                                          new_name=workflow_name,
                                                          new_quota_project_id=self.mode.quota_project_id,
                                                          new_project_code=self.mode.project_code)

            workflow_params = {}
            if self.mode.workflow_permissions:
                workflow_params["permissions"] = self.mode.workflow_permissions
            if self.mode.workflow_description:
                workflow_params["description"] = self.mode.workflow_description

            self.proxy.edit_workflow(workflow_id=self.workflow_id,
                                     workflow_instance_id=self.instance_id,
                                     workflow_params=workflow_params)
            logging.info("New workflow %s was successfully created", self.get_workflow_url())
        except Exception as e:
            raise WorkflowCreationError("workflow creation error: {}".format(e))

    def setup_workflow(self):
        logging.info("Setting global parameters...")
        try:
            global_params = self.mode.construct_supported_params(self.get_global_params())
            status = self.proxy.set_global_parameters(workflow_id=self.workflow_id,
                                                      global_parameters=global_params,
                                                      workflow_instance_id=self.instance_id)
            logging.info("Status of setting global graph parameters: %s", status)
        except Exception as e:
            raise SetGlobalParametersError("workflow setup global parameters error: {}".format(e))
        logging.info("Validating workflow...")
        try:
            self.proxy.validate_workflow(workflow_id=self.workflow_id,
                                         workflow_instance_id=self.instance_id)
            logging.info("Workflow was successfully validated")
        except Exception as e:
            raise WorkflowValidationError("workflow validation error: {}".format(e))

    def start_workflow(self):
        logging.info("Starting workflow...")
        try:
            self._instance_id = self.proxy.start_workflow_once(workflow_id=self.workflow_id,
                                                               workflow_instance_id=self.instance_id)
            logging.info("Workflow %s successfully started", self.get_workflow_url())
        except Exception as e:
            raise WorkflowStartingError("workflow starting error: {}".format(e))

    def wait_for_workflow(self):
        progress_logger = lambda workflow_id, instance_id, status, progress: write_graph_progress(progress)
        time_start = time.time()
        write_graph_progress(0)
        try:
            result, _ = self.proxy.wait_for_workflow_completion(workflow_id=self.workflow_id,
                                                                workflow_instance_id=self.instance_id,
                                                                wait_timeout=self.timeout,
                                                                progress_callback=progress_logger)
        except Exception as e:
            raise WorkflowExecutionError("workflow execution error: {}".format(e))
        if result != "success":
            raise WorkflowExecutionError("workflow execution error: graph returned '{}'".format(result))
        write_graph_progress(1)
        time_end = time.time()
        logging.info("Workflow %s was executed in %s", self.get_workflow_url(),
                     datetime.timedelta(seconds=time_end - time_start))

    def get_graph_result(self):
        logging.info("Getting graph result url...")
        try:
            storage_path = self.proxy.get_block_output_url(workflow_id=self.workflow_id,
                                                           workflow_instance_id=self.instance_id,
                                                           block_code="mstand_ConvertToABFormat",
                                                           output_name="converted_pool")
        except Exception as e:
            raise WorkflowResultDownloadingError("workflow result downloading error: {}".format(e))

        if not storage_path:
            raise WorkflowResultDownloadingError("storage path is empty")

        logging.info("Graph result url: %s", storage_path)
        logging.info("Downloading result...")
        try:
            request = urllib2.Request(
                storage_path,
                headers={"Authorization": "OAuth {}".format(self.proxy.token)}
            )
            result_stream = urllib2.urlopen(request, timeout=120)
            result = json.load(result_stream)
        except Exception:
            raise WorkflowResultDownloadingError("storage path download error")
        logging.info("Raw results were downloaded", result)
        return result

    def run(self):
        self.create_new_workflow()
        self.setup_workflow()
        self.start_workflow()
        self.wait_for_workflow()
        raw_result = self.get_graph_result()
        result = self.mode.add_meta(raw_result, self.get_workflow_url())
        return result

    def stop(self):
        if not self.workflow_id or not self.instance_id:
            return

        try:
            self.proxy.stop_workflow(workflow_id=self.workflow_id,
                                     workflow_instance_id=self.instance_id)
            logging.info("Workflow %s was successfully stopped", self.get_workflow_url())
        except Exception as e:
            logging.error("Failed to stop workflow %s: %s", self.get_workflow_url(), e, exc_info=True)
            raise
