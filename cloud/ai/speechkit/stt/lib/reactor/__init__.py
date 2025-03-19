import reactor_client as r

# https://reactor.yandex-team.ru/browse?selected=8562657

project_identifier = r.r_objs.ProjectIdentifier(project_id=5582)
reaction_owner = 'robot-cloud-ai'


def get_markup_artifact_identifier_for_trigger(name: str) -> r.r_objs.ArtifactIdentifier:
    return r.r_objs.ArtifactIdentifier(namespace_identifier=get_markup_artifact_namespace_for_trigger(name))


def get_markup_artifact_reference_for_trigger(name: str) -> r.r_objs.ArtifactReference:
    return r.r_objs.ArtifactReference(namespace_identifier=get_markup_artifact_namespace_for_trigger(name))


def get_markup_artifact_namespace_for_trigger(name: str) -> r.r_objs.NamespaceIdentifier:
    return r.r_objs.NamespaceIdentifier(
        namespace_path=get_markup_artifact_path(name)
    )


def get_markup_artifact_path(name: str):
    return f'/cloud/ai/speechkit/stt/toloka-transcription-pipeline/trigger-artifacts/{name}'
