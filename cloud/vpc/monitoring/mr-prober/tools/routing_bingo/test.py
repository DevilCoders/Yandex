import dataclasses
from datetime import datetime
import logging
import operator
import os
import time
from typing import List, Dict, Any, Optional

import pydantic
import yaml
from pydantic import BaseModel, Protocol, conlist
from teamcity.messages import TeamcityServiceMessages

import api
from api.client import MrProberApiClient
from tools.routing_bingo.config import Environment, Cluster
from tools.routing_bingo.solomon import SolomonClient, SolomonProberResultVector

TESTS_GLOB = "test_*.yaml"


class TestRun(BaseModel):
    name: str
    skip: Optional[str] = None
    variables: Dict[str, Any] = {}


class TestSpec(BaseModel):
    cluster_slug: str
    prober_result_count: int = 3
    prober_interval: int
    prober_slugs: conlist(str, min_items=1)
    extra_labels: Dict[str, str] = {}
    base_variables: Dict[str, Any] = {}
    runs: List[TestRun] = {}


@dataclasses.dataclass
class Test:
    """
    The test contains either spec/cluster if it is successfully loaded or
    error - in that case Test is used only as stub that report errors
    """
    name: str

    spec: TestSpec
    cluster: Cluster
    solomon: SolomonClient

    error: Optional[str] = None


@dataclasses.dataclass
class TestRunTimeFrame:
    run: TestRun
    start_time: float
    end_time: float


class TestRunner:
    INITIAL_POLL_INTERVAL, INITIAL_POLL_ATTEMPTS = 20.0, 5
    POLL_INTERVAL, POLL_ATTEMPTS = 5.0, 30
    ALIVE_INTERVAL, ALIVE_ATTEMPTS = 30, 6

    def __init__(self, test: Test, client: MrProberApiClient, solomon: SolomonClient):
        self.test = test
        self.client = client
        self.solomon = solomon

        self.msg = TeamcityServiceMessages()

    def run(self) -> bool:
        has_failures = False

        if self.test.error:
            self._report_test_error(self.test.name, f"Error loading test: {self.test.error}")
            logging.error(f"Error loading test {self.test.name}. {self.test.error}.")
            return

        # There is a small delay between metrics reading and them becoming available in solomon /data
        # handle, so we do not run check immediately, instead we continue deploying new variants
        # of tests and then read all historical data
        run_time_frames: List[TestRunTimeFrame] = []
        for run in self.test.spec.runs:
            tf = self.setup_test(run)
            if tf is not None:
                run_time_frames.append(tf)
            else:
                has_failures = True

        for tf in run_time_frames:
            # FIXME: that would be faster to query solomon once and split it into chunks by time here
            try:
                results = self.solomon.get_prober_results((tf.start_time, tf.end_time),
                                                          self.test.spec.extra_labels,
                                                          self.test.spec.prober_slugs,
                                                          self.test.spec.prober_result_count)
                if len(results) == 0:
                    raise ValueError("No prober results are found")

                has_run_failures = self.check_test_run(tf, results)
                has_failures = has_failures or has_run_failures
            except Exception as e:
                self._report_test_error(f"{self.test.name}/{run.name}:check",
                                        f"Error getting test results: {self.test.error}")

                logging.error(f"Error in test {self.test.name}/{tf.run.name}. {e}")

        return has_failures

    def _report_test_error(self, tc_run_name: str, error: str):
        self.msg.testStarted(tc_run_name)
        self.msg.testFailed(tc_run_name, message=error)
        self.msg.testFinished(tc_run_name)

    def setup_test(self, run: TestRun) -> Optional[TestRunTimeFrame]:
        run_name = f"{self.test.name}/{run.name}"
        tc_run_name = f"{run_name}:setup"

        self.msg.testStarted(tc_run_name)
        if run.skip is not None:
            logging.info(f"Test {run_name} is skipped: {run.skip}")
            self.msg.testIgnored(tc_run_name, message=run.skip)
            self.msg.testFinished(tc_run_name)
            return None

        start_time = time.time()
        logging.info(f"Starting test {run_name}")
        try:
            self.update_cluster(run)

            result_count = self.test.spec.prober_result_count
            time_delta = self.test.spec.prober_interval * (result_count + 0.5)
            logging.info(f"Waiting for {int(time_delta)}s / {result_count} prober runs")
            time.sleep(time_delta)
        except Exception as e:
            logging.error(f"Error in test {run_name}: {e}")

            self.msg.testFailed(tc_run_name, str(e))
            self.msg.testFinished(tc_run_name)
            return None
        else:
            duration = datetime.now() - datetime.utcfromtimestamp(start_time)
            self.msg.testFinished(tc_run_name, testDuration=duration)

        return TestRunTimeFrame(run, start_time, start_time + time_delta)

    def update_cluster(self, run: TestRun):
        cluster = self.client.clusters.get(self.test.cluster.cluster_id)

        # FIXME: this will probably update all cluster variable rows in db
        new_variables = {**self.test.spec.base_variables, **run.variables}
        variables = {v.name: v.value for v in cluster.variables}

        diff_variables = set()
        for variable, new_value in new_variables.items():
            old_value = variables.get(variable)
            if old_value != new_value:
                diff_variables.add(variable)

        if not diff_variables:
            logging.info(f"Skipping cluster update for cluster {self.test.spec.cluster_slug}, "
                         "all variables are already set")
            self._wait_cluster_alive()
            return

        variables.update(new_variables)
        req = api.models.UpdateClusterRequest(
            manually_created=False,
            arcadia_path=cluster.arcadia_path,
            recipe_id=cluster.recipe.id,
            name=cluster.name,
            slug=cluster.slug,
            variables=variables,
        )

        logging.info(f"Updating cluster {self.test.spec.cluster_slug} with variables {new_variables}")
        self.client.clusters.update(cluster.id, req)

        for attempt in range(self.INITIAL_POLL_ATTEMPTS + self.POLL_ATTEMPTS):
            current_cluster = self.client.clusters.get(self.test.cluster.cluster_id)
            redeploy_time = current_cluster.last_deploy_attempt_finish_time
            if redeploy_time > cluster.last_deploy_attempt_finish_time:
                logging.info(f"Cluster {self.test.spec.cluster_slug} was redeployed on {redeploy_time}")
                break

            logging.debug(f"Waiting for cluster {self.test.spec.cluster_slug} redeploy...")
            time.sleep(self.INITIAL_POLL_ATTEMPTS if attempt < self.INITIAL_POLL_ATTEMPTS
                       else self.POLL_INTERVAL)
        else:
            raise RuntimeError(f"Timed out waiting for cluster {self.test.spec.cluster_slug} redeploy")

        self._wait_cluster_alive()

    def _wait_cluster_alive(self):
        # FIXME: this doesn't check if targets are redeployed, only agents
        time.sleep(self.ALIVE_INTERVAL / 2)
        for attempt in range(self.ALIVE_ATTEMPTS * 2):
            if self._check_cluster_alive():
                break

            time.sleep(self.ALIVE_INTERVAL / 2)
        else:
            raise RuntimeError(f"Timed out waiting for agent {self.test.spec.cluster_slug} alive")

    def _check_cluster_alive(self) -> bool:
        cluster_slug = self.test.spec.cluster_slug
        now = time.time()
        metrics = self.solomon.get_last_metrics((now - 1.5 * self.ALIVE_INTERVAL, now), {
            "cluster_slug": cluster_slug,
            "metric": "keep_alive",
        })

        if len(metrics.vector) == 0:
            logging.warning(f"Keep alive checker for cluster {cluster_slug} had returned empty set of metrics")
            return False

        ts = metrics.vector[0].timeseries
        if len(ts.values) == 0 or any(val < 1.0 for val in ts.values):
            logging.warning(f"Keep alive checker for cluster {cluster_slug} had returned empty timeseries")
            return False

        logging.info(f"Cluster {cluster_slug} is alive!")
        return True

    def check_test_run(self, tf: TestRunTimeFrame, results: List[SolomonProberResultVector]) -> bool:
        results_with_names = []
        for result in results:
            test_result_tags = filter(
                None, [
                    self.test.name, tf.run.name,
                    result.prober_slug if len(self.test.spec.prober_slugs) > 1 else None,
                    ",".join(f"{param}={value}" for param, value in
                        sorted(result.params.items())) if len(result.params) > 0 else None
                ])
            results_with_names.append(("/".join(test_result_tags), result))

        has_unsuccessful_results = False
        for test_result_name, result in sorted(results_with_names, key=operator.itemgetter(0)):
            tc_result_name = f"{test_result_name}:check"
            self.msg.testStarted(tc_result_name)

            unsuccessful_results = result.unsuccessful_results()
            if len(unsuccessful_results) == 0:
                logging.info(f"Test {test_result_name} has passed")
                self.msg.testFinished(tc_result_name)
                continue

            for unsuccessful_result in unsuccessful_results:
                logging.warning(f"Test {test_result_name} has failed at {unsuccessful_result.timestamp.ctime()}"
                                f" with status = {unsuccessful_result.status}")

            self.msg.testFailed(tc_result_name, message=f"Test got {len(unsuccessful_results)} unsuccessful results")
            self.msg.testFinished(tc_result_name)
            has_unsuccessful_results = True

        return has_unsuccessful_results


def load_tests(test_paths: List[str], environment: Environment) -> List[Test]:
    tests = []
    for test_path in test_paths:
        name, _ = os.path.splitext(os.path.basename(test_path))
        error, spec = None, None
        try:
            raw_cfg = pydantic.parse.load_file(test_path, proto=Protocol.json,
                                               json_loads=yaml.safe_load)
            spec = TestSpec.parse_obj(raw_cfg)
        except FileNotFoundError as e:
            logging.error(f"Test file {test_path} is not found, omitting")
            error = e
        except (pydantic.ValidationError, yaml.constructor.ConstructorError) as e:
            logging.error(f"Cannot parse test file {test_path}: {e}, omitting")
            error = e

        if spec:
            # TODO: investigate whether we can simply list all clusters from Mr.Prober API and build this data ourselves
            cluster = environment.clusters.get(spec.cluster_slug)
            if not cluster:
                error = Exception(f"Cannot find cluster {spec.cluster_slug}")
        else:
            cluster = None

        tests.append(Test(name, spec, cluster, error))

    return tests
