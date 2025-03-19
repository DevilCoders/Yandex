import vh
from pathlib import Path
import logging

logger = logging.getLogger(__name__)


@vh.module(sandbox_token=vh.Secret, sandbox_owner=str, layer_name=str, script=vh.File, parent_layer=str, layer_id=vh.mkoutput(vh.File))
def _get_porto_layer(sandbox_token, sandbox_owner, layer_name, script, parent_layer, layer_id):  # type: ignore
    build_porto_layer_op = vh.op(id='c212427e-9dfc-4a29-a220-73880755f620')
    build_porto_layer_op(
        script=script,
        layer_id=layer_id,
        parent_layer=parent_layer,
        name=layer_name,
        sandbox_token=sandbox_token,
        sandbox_group=sandbox_owner)


def build_nirvana_layer(
        environment_script: str,
        sandbox_token: str,
        layer_name: str,
        sandbox_owner: str = 'CLOUD_ANALYTICS',
        quota: str = 'default',
        local_file_for_layer_id: str = 'porto_layer_id.txt',
        parent_layer: str = 'c6ab33cc-40ae-4479-b706-5e1596b90be4'
        ) -> None:
    """Function to build custom porto layers to use them in nirvana.
       Saves id of layer to local file to reuse it in future. By default uses Ubuntu 18.04 as base layer.
       It is better to add this file to Arc.

    :param environment_script: Script to set up porto layer as string
    :type environment_script: str
    :param sandbox_token: Token for Sandboc from Nirvana vault, defaults to 'example-sandbox-token'
    :type sandbox_token: str, optional
    :param sandbox_owner: Owner of job which build porto layer, defaults to 'CLOUD_ANALYTICS'
    :type sandbox_owner: str, optional
    :param layer_name: Name for layer. Better to describe environment somehow, defaults to 'example'
    :type layer_name: str, optional
    :param quota: Quota to build layer. If job fails. Try to change to different,
                  for example to 'external-activities', defaults to 'default'
    :type quota: str, optional
    :param local_file_for_layer_id: Local file name to save layer id, defaults to 'porto_layer_id.txt'
    :type local_file_for_layer_id: str, optional
    """
    logger.info('Started to build layer')
    layer_id = _get_porto_layer(
        script=vh.data_from_str(environment_script, name='environment_script'),
        sandbox_token=sandbox_token,
        sandbox_owner=sandbox_owner,
        parent_layer=parent_layer,
        layer_name=layer_name).layer_id
    keeper = vh.run(wait=True, label=layer_name,
                    description=f'Building porto layer {layer_name}',
                    quota=quota)
    layer_id_path = keeper.download(layer_id, path=local_file_for_layer_id)
    layer_id_str = Path(layer_id_path).read_text()
    workflow_info = keeper.get_workflow_info()
    logger.info(f'Workflow: https://nirvana.yandex-team.ru/flow/'
                f'{workflow_info.workflow_id}/{workflow_info.workflow_instance_id}')
    logger.info(f'Layer: https://nirvana.yandex-team.ru/layer/{layer_id_str}')
