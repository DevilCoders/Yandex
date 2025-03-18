import startrek_client as st

from ci.registry.junk.bsheludko.update_bio_beta_tasklet.proto import update_bio_beta_tasklet as bio_tasklet_proto

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


def append_sandbox_resource(patch, path, resource_id):
    append_patch_impl(patch, path, resource_id, Patch.Resource.ManageType.SANDBOX_RESOURCE)


def make_uniproxy_url(yabio_balancer: str, legacy_redirections: "list[str]") -> str:
    params_list = [f"srcrwr={r}:{yabio_balancer}:10701" for r in legacy_redirections]
    return UNIPROXY_HAMSTER + "?" + "&".join(params_list)


class BioTaskletImpl(bio_tasklet_proto.UpdateBIOBetaBase):
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

    def get_bio_server_pods(self):
        return get_component_pods_by_name(self.beta, self.input.config.cpu_component_name)

    def get_bio_server_balancer(self):
        if self.input.config.yabio_balancer:
            return self.input.config.yabio_balancer
        pods = self.get_bio_server_pods()
        return pods[0]

    def get_uniproxy_url(self):
        yabio_balancer = self.get_bio_server_balancer()
        return make_uniproxy_url(yabio_balancer, self.input.config.legacy_redirections)

    def make_bio_server_cpu_patch(self):
        cpu_component_id = get_component_id_by_name(self.beta, self.input.config.cpu_component_name)
        assert cpu_component_id

        cpu_patch = UpdatePatchesProto.PatchMap(id=cpu_component_id)

        cpu_patch.patch.parent_external_id = self.input.config.bio_server_parent_service_id

        if self.input.config.lingware_resource_id:
            append_sandbox_resource(cpu_patch.patch, "lingware", self.input.config.lingware_resource_id)

        if self.input.config.bio_server_cpu_resource_id:
            append_sandbox_resource(cpu_patch.patch, "bio-server", self.input.config.bio_server_cpu_resource_id)

        if self.input.config.bio_server_cpu_runner_resource_id:
            append_sandbox_resource(cpu_patch.patch, "run.sh", self.input.config.bio_server_cpu_runner_resource_id)

        if self.input.config.bio_server_cpu_evlogdump_resource_id:
            append_sandbox_resource(cpu_patch.patch, "evlogdump", self.input.config.bio_server_cpu_evlogdump_resource_id)

        return cpu_patch

    def run(self):
        logger.info(f"Input {self.input}")

        self.init()

        cpu_patch = self.make_bio_server_cpu_patch()
        assert cpu_patch

        patches = [cpu_patch]
        
        logger.info(f"Patches: {patches}")

        self.output.output.uniproxy_url = self.get_uniproxy_url()
        self.output.output.host_address_for_stress_test = self.get_bio_server_balancer()

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
