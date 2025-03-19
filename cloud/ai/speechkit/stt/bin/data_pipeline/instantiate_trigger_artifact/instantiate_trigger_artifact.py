import typing
from datetime import datetime

import nirvana.job_context as nv
import reactor_client as r

import cloud.ai.speechkit.stt.lib.reactor as reactor_consts


def main():
    params = nv.context().parameters

    artifact: typing.Optional[str] = params.get('artifact')
    reactor_token: str = params.get('reactor-token')

    if not artifact:
        return

    client = r.ReactorAPIClientV1(base_url='https://reactor.yandex-team.ru', token=reactor_token)

    client.artifact_instance.instantiate(
        artifact_identifier=reactor_consts.get_markup_artifact_identifier_for_trigger(artifact),
        metadata=r.r_objs.Metadata(type_="/yandex.reactor.artifact.EventArtifactValueProto", dict_obj={}),
        user_time=datetime.now(),
        create_if_not_exist=True,
    )
