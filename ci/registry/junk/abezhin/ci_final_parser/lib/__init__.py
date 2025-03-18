from ci.registry.junk.abezhin.ci_final_parser.proto import ci_final_parser_tasklet
from tasklet.services.yav.proto import yav_pb2 as yav
import sandbox.sdk2 as sandbox_sdk2
from sandbox import common
from ci.registry.junk.abezhin.ci_final_parser.lib.nirvana import Workflow as NirvanaWorkflow

import yt.wrapper as yt
import yt.yson as yson
import pulsar

import requests
import io
import tarfile
import json
import logging
import time
import typing
import datetime


def make_logger():
    # create logger
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    # create console handler and set level to debug
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)

    # create formatter
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    # add formatter to ch
    ch.setFormatter(formatter)

    # add ch to logger
    logger.addHandler(ch)
    return logger


logger = make_logger()


class VOICETECH_ASR_STRESS_TEST_RESULT(sandbox_sdk2.Resource):
    pass





@yt.yt_dataclass
class MiniReleaseReport:
    name: str
    commit_number: int
    date: str
    data: typing.Optional[yt.schema.YsonBytes] = None


def process_stress_test_report(tar_file_contents):
    def extract_rps_limit(json_report):
        prev_rps = 0
        for k, v in json_report.items():
            prefix, rps = k.split(":")
            assert prefix == "rps"
            rps = int(rps)

            v = v["asr"]
            req_count_total = v['total_count']
            req_count_503 = v['err_503_count']
            share_503 = req_count_503 / float(req_count_total)

            logger.info(f"RPS {rps}, Total Count={req_count_total}, 503 count={req_count_503}")

            if share_503 > 0.01:
                break
            prev_rps = rps
        return float(prev_rps)

    def process_single_report(tar_file_obj, report_name):
        try:
            tar_file_obj.getmember(report_name)
        except KeyError:
            return None

        report = tar_file_obj.extractfile(report_name).read()
        report = json.loads(report)
        return extract_rps_limit(report)

    with io.BytesIO(tar_file_contents) as tarred_data_file:
        tf = tarfile.open(fileobj=tarred_data_file)
        rps_limit = process_single_report(tf, "output_directory/release/report.json")
        rps_limit_degr = process_single_report(tf, "output_directory/release_degradation/report.json")
        return rps_limit, rps_limit_degr


def get_workflow_and_instance_from_url(url):
    workflow_id, instance_id = url.split("nirvana.yandex-team.ru/flow/")[-1].split("/")[0:2]
    return workflow_id, instance_id


def wait_ue2e(url, nirvana_token, minutes=None):
    workflow_id, instance_id = get_workflow_and_instance_from_url(url)
    wf = NirvanaWorkflow(nirvana_token, workflow_id)
    logger.info('Waiting for url: ' + url)
    if minutes is not None:
        assert isinstance(minutes, int)
        assert minutes >= 0

    while True:
        status, result, progress = wf.instance_status(instance_id)
        if status == 'completed':
            logger.info(f"Finished. Result {result}")
            break
        else:
            logger.info(f"Status {status}, progress {progress}")

            if minutes is not None:
                if minutes == 0:
                    logger.info(f"Wait is too long")
                    return False
                minutes -= 1

            time.sleep(60)

    return True
    # return wf.get_block_results(instance_id, "operation-results-viewer-downloader-metrics", "out1")


def get_resource_data(sbr_id):
    resource = sandbox_sdk2.Resource.find(id=sbr_id).first()
    resp = requests.get(resource.http_proxy, params={'stream': 'tar'})
    assert resp.status_code == 200

    return resp.content


class FinalParserImpl(ci_final_parser_tasklet.CIFinalizeBase):
    def get_nirvana_token(self):
        spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key="nirvana_oauth_token")
        return self.ctx.yav.get_secret(spec).secret

    def get_yt_token(self):
        spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key="yt.token")
        return self.ctx.yav.get_secret(spec).secret

    def make_report(self):
        max_rps = None
        max_rps_degradation = None
        for sbr in self.input.sb_resources:
            if sbr.type == 'VOICETECH_ASR_STRESS_TEST_RESULT':
                data = get_resource_data(sbr.id)
                max_rps, max_rps_degradation = process_stress_test_report(data)

        data = {
            "max_rps": max_rps,
            "max_rps_degradation": max_rps_degradation
        }
            

        return MiniReleaseReport(
            date=datetime.datetime.now().isoformat(),
            name=self.input.context.config_info.id,
            commit_number=self.input.context.target_revision.number,
            data=yson.dumps(data)
        )
        
    def run(self):
        ue2e_res = None
        if url := self.input.config.ue2e_instance_url:
            nirvana_token = self.get_nirvana_token()
            ue2e_res = wait_ue2e(url, nirvana_token, None)
            if not ue2e_res:
                raise common.errors.TaskFailure("Graph wait operation timed out")

        if self.input.config.yt_table_for_report:
            report = self.make_report()

            client = yt.YtClient(
                proxy=self.input.config.yt_proxy_for_report,
                token=self.get_yt_token())

            table_exists = client.exists(self.input.config.yt_table_for_report)
            table = yt.TablePath(self.input.config.yt_table_for_report, append=table_exists)    
            client.write_table_structured(table, MiniReleaseReport, [report])

            client.run_merge(
                self.input.config.yt_table_for_report, 
                self.input.config.yt_table_for_report, 
                spec={"combine_chunks": True})

        self.output.state.success = True

        # res = json.loads(res)

        # for i in res:
        #     basket_name = i.get('basket')
        #     if not basket_name:
        #         continue

        #     metric_name = i["metric_name"]

        #     if metric_name in ('integral', 'integral_on_asr_changed'):
        #         print(basket_name, metric_name, i['prod_quality'], i['test_quality'], i['pvalue'])

        # self.output.state.success = False

        # try:
        #     print(self.input)
        #     print(self.context)
        # except:
        #     pass

        # logger.info(f"Getting secret from {self.input.context.secret_uid}")
        # spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key="pulsar.token")
        # pulsat_token = self.ctx.yav.get_secret(spec).secret
        # logger.info("Got pulsar token")

        # logger.info(f"Downloading resource {self.input.sb_resources[0].id}")
        # resource = sandbox_sdk2.Resource.find(id=self.input.sb_resources[0].id).first()
        # resp = requests.get(resource.http_proxy, params={'stream': 'tar'})
        # logger.info(f"Status code  {resp.status_code}")

        # assert resp.status_code == 200

        # metrics = {}

        # with io.BytesIO(resp.content) as tarred_data_file:
        #     tf = tarfile.open(fileobj=tarred_data_file)

        #     metrics['rps'] = process_report(tf, 'output_directory/release/report.json')
        #     metrics['rps_degradation'] = process_report(tf, 'output_directory/release_degradation/report.json')

        # pulsar_model = pulsar.ModelInfo(name=self.input.config.model)

        # pulsar_dataset = pulsar.DatasetInfo(name="stress_test")
        # pulsar_instance = pulsar.InstanceInfo(
        #     model=pulsar_model,
        #     dataset=pulsar_dataset,
        #     result=metrics
        # )

        # pulsar_client = pulsar.PulsarClient(token=pulsat_token)
        # instance_id = pulsar_client.add(pulsar_instance)

        # logger.info(f"Pulsar model={pulsar_model}, dataset={pulsar_dataset}, instance={instance_id}")
        # logger.info(f"Plot url: https://pulsar.yandex-team.ru/plot?dataset={pulsar_dataset.name}&model={pulsar_model.name}")

        # self.output.state.success = True
