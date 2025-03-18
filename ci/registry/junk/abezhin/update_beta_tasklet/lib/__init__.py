from ci.registry.junk.abezhin.update_beta_tasklet.proto import update_asr_beta_tasklet as tasklet_proto
import startrek_client as st


from search.priemka.yappy.services.yappy.services import Lineage2HttpClient, Model
from search.priemka.yappy.proto.structures.payload_pb2 import Lineage2Response
from search.priemka.yappy.proto.structures.patch_pb2 import Patch
from search.priemka.yappy.proto.structures.beta_pb2 import BetaFilter
from search.priemka.yappy.proto.structures.status_pb2 import Status
from search.priemka.yappy.proto.structures.api_pb2 import ApiQuotaFilter, UpdatePatches as UpdatePatchesProto, ApiBetaFilter


from sandbox import common
from tasklet.services.yav.proto import yav_pb2 as yav

import time
import logging


DEFAULT_YAPPY_URL = "https://yappy.z.yandex-team.ru/api"
UNIPROXY_HAMSTER = "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws"


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


def get_component_id_by_name(beta, name):
    assert name
    for component in beta.components:
        if component.type.name == name:
            return component.id
    return None


def get_component_pods_by_name(beta, name):
    assert name
    for component in beta.components:
        if component.type.name == name:
            return list([i.split(":")[0] for i in component.slot.instances])

    return []


def append_patch_impl(patch, path, value, type):
    if not value:
        return

    if type == Patch.Resource.ManageType.SANDBOX_RESOURCE:
        patch.resources.append(
            Patch.Resource(
                manage_type=type, local_path=path, sandbox_resource_id=value
            )
        )
    elif type == Patch.Resource.ManageType.STATIC_CONTENT:
        patch.resources.append(
            Patch.Resource(
                manage_type=type, local_path=path, content=value
            )
        )
    else:
        raise Exception()


def append_static_content(patch, path, content):
    append_patch_impl(patch, path, content, Patch.Resource.ManageType.STATIC_CONTENT)


def append_sandbox_resource(patch, path, resource_id):
    append_patch_impl(patch, path, resource_id, Patch.Resource.ManageType.SANDBOX_RESOURCE)


def make_mixed_runner_config(endpoint_set, cluster):
    assert endpoint_set and cluster
    return f"""{{
"remote_runner_config": {{
    "max_request_error_attempts": 1, 
    "backend_type": "yp_sd", 
    "balancer_worker_count": 8,
    "grpc_keepalive_ping_period": "1s",
    "max_crash_error_attempts": 1,
    "backend": {{
        "cluster": "{cluster}", 
        "service": "{endpoint_set}", 
        "sd_cache_dir": "sd_cache"
    }}, 
    "enable_grpc_keepalive_ping": 1,
    "max_fast_error_attempts": 3, 
    "startup_check": {{
        "ensure_same_source_version": true, 
        "ensure_same_dc": false, 
        "enabled": true
    }}, 
    "timeout": "60s", 
    "grpc_max_channels_to_backend": 8,
    "grpc_keepalive_ping_timeout": "100ms", 
    "grpc_threads": 8, 
    "max_resolution_attempts": 1, 
    "executors_count": 8, 
    "max_attempts": 1
}},

"degradation_trigger_params": {{
    "degradation_time_ms": 60000, 
    "thresholds": {{
        "cpu_threshold": 0.7, 
        "gpu_usage_threshold": 0.5, 
        "nn_runner_sessions_threshold": 150
    }}
}}, 

"gpu_distribution_strategy": "EverythingToEverybody"
}}"""



def make_uniproxy_url(
    pods: "list[str]", apphost_node: str, yaldi_balancer: str, legacy_redirections: "list[str]"
) -> str:
    params_list = []
    apphost_backends = list(f"{p}:10501" for p in pods)
    params_list.append(f"ahsrcrwr={apphost_node}:{';'.join(apphost_backends)}")

    for r in legacy_redirections:
        params_list.append(f"srcrwr={r}:{yaldi_balancer}")

    return UNIPROXY_HAMSTER + "?" + "&".join(params_list)


class TaskletImpl(tasklet_proto.UpdateASRBetaBase):
    def init(self):
        logger.info(f"Getting secret from {self.input.context.secret_uid}")
        spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key="yappy.token")
        token = self.ctx.yav.get_secret(spec).secret

        self.model = Model.ModelHttpClient(base_url=DEFAULT_YAPPY_URL, oauth_token=token)
        self.yappy = Lineage2HttpClient(base_url=DEFAULT_YAPPY_URL, oauth_token=token)

        beta_name = self.input.config.beta_name
        assert beta_name

        beta_exists = self.model.beta_exists(BetaFilter(name=beta_name)).exists
        msg = f"Beta name {beta_name} {'exists' if beta_exists else 'does not exist'}"
        logger.info(msg)
        if not beta_exists:
            raise common.errors.TaskFailure(msg)

        self.beta = self.model.retrieve_beta(ApiBetaFilter(name=beta_name))


    def wait(self, minutes):
        if not minutes:
            return True

        logger.info("Waiting beta")
        time.sleep(30)

        retry_counter = 0
        while retry_counter < minutes:
            status = self.model.get_beta_status(BetaFilter(name=self.input.config.beta_name))
            if status.state.status == Status.CONSISTENT:
                logger.info(f"Beta is consistent")
                return True

            logger.info(f"Beta is not consistent yet. Sleeping for 1 min (retry {retry_counter})")
            time.sleep(60)
            retry_counter += 1

        logger.info('Beta creation took too long')
        return False


    def post_comment(self, txt):
        if not self.input.config.release_ticket:
            return

        st_token_spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key='startrek.token')
        st_token = self.ctx.yav.get_secret(st_token_spec).secret
        client = st.Startrek(useragent='GetOrCreateStTicket', token=st_token)

        ticket_obj = client.issues[self.input.config.release_ticket]
        ticket_obj.comments.create(text=txt)

    def get_asr_server_pods(self):
        component_name = self.input.config.gpu_component_name if self.input.config.monolith else self.input.config.cpu_component_name
        pods = get_component_pods_by_name(self.beta, component_name)

        return pods

    def get_asr_server_balancer(self):
        if self.input.config.yaldi_balancer:
            return self.input.config.yaldi_balancer

        pods = self.get_asr_server_pods()
        return pods[0]

    def get_uniproxy_url(self):
        pods = self.get_asr_server_pods()
        yaldi_balancer = self.get_asr_server_balancer()

        return make_uniproxy_url(
                pods, self.input.config.apphost_node, yaldi_balancer, self.input.config.legacy_redirections
            )

    def make_asr_server_gpu_patch(self):
        assert self.input.config.monolith
        gpu_component_id = get_component_id_by_name(self.beta, self.input.config.gpu_component_name)
        gpu_patch = UpdatePatchesProto.PatchMap(id=gpu_component_id)

        gpu_patch.patch.parent_external_id = self.input.config.asr_server_parent_service_id

        append_sandbox_resource(gpu_patch.patch, "lingware", self.input.config.lingware_resource_id)
        append_sandbox_resource(gpu_patch.patch, "asr-server", self.input.config.asr_server_gpu_resource_id)
        append_static_content(gpu_patch.patch, "nn_server_config.json.tmpl", "(Make Yappy Happy)")
        
        return gpu_patch


    def make_asr_server_cpu_patch(self):
        assert not self.input.config.monolith
        cpu_component_id = get_component_id_by_name(self.beta, self.input.config.cpu_component_name)

        assert cpu_component_id
        cpu_patch = UpdatePatchesProto.PatchMap(id=cpu_component_id)

        cpu_patch.patch.parent_external_id = self.input.config.asr_server_parent_service_id

        append_sandbox_resource(cpu_patch.patch, "lingware", self.input.config.lingware_resource_id)
        append_sandbox_resource(cpu_patch.patch, "asr-server", self.input.config.asr_server_cpu_resource_id)
        append_static_content(cpu_patch.patch, "mixed_runner_config.json.tmpl", make_mixed_runner_config(
            self.input.config.nn_server_endpointset_name,
            self.input.config.nn_server_endpointset_dc
        ))

        return cpu_patch


    def make_empty_cpu_patch(self):
        assert self.input.config.monolith
        if not self.input.config.cpu_component_name:
            return None

        cpu_component_id = get_component_id_by_name(self.beta, self.input.config.cpu_component_name)
        if not cpu_component_id:
            return None

        cpu_patch = UpdatePatchesProto.PatchMap(id=cpu_component_id)

        # A special "Empty" service https://nanny.yandex-team.ru/ui/#/services/catalog/asr-ci-empty-cpu
        cpu_patch.patch.parent_external_id = "asr-ci-empty-cpu"

        return cpu_patch


    def make_nn_server_patch(self):
        assert not self.input.config.monolith
        gpu_component_id = get_component_id_by_name(self.beta, self.input.config.gpu_component_name)
        gpu_patch = UpdatePatchesProto.PatchMap(id=gpu_component_id)

        gpu_patch.patch.parent_external_id = self.input.config.nn_server_parent_service_id
        append_sandbox_resource(gpu_patch.patch, "lingware", self.input.config.lingware_resource_id)
        append_sandbox_resource(gpu_patch.patch, "nn_server", self.input.config.nn_server_resource_id) 

        return gpu_patch

    

    def run(self):
        logger.info(f"Input {self.input}")
        self.init()

        cpu_patch = None
        gpu_patch = None

        if self.input.config.monolith:
            cpu_patch = self.make_empty_cpu_patch()
            gpu_patch = self.make_asr_server_gpu_patch()
        else:
            cpu_patch = self.make_asr_server_cpu_patch()
            assert cpu_patch

            gpu_patch = self.make_nn_server_patch()

        patches = [gpu_patch]
        if cpu_patch:
            patches.append(cpu_patch)
        
        logger.info(f"Patches: {patches}")

        self.output.output.uniproxy_url = self.get_uniproxy_url()
        self.output.output.host_address_for_stress_test = self.get_asr_server_balancer()

        output = self.yappy.update_patches(request=UpdatePatchesProto(beta_name=self.input.config.beta_name, patches=patches))
        logger.info(f"Update patches output: {output}")

        exception_message = ""
        if output.status != Lineage2Response.Status.SUCCESS:
            exception_message = 'Beta creation failed: ' + str(output)

        if not exception_message:
            if not self.wait(self.input.config.wait_beta_minutes):
                exception_message = 'Beta creation took too long'

        comment = f"""Updating beta %%{self.input.config.beta_name}%%. Uniproxy url: %%{self.output.output.uniproxy_url}%%"""
        if exception_message:
            comment += "\n\nException: " + exception_message

        self.post_comment(comment)

        if exception_message:
            raise common.errors.TaskFailure(exception_message)
