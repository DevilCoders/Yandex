import os
import logging
import time

logging.basicConfig()
log = logging.getLogger(__name__)

log.setLevel(logging.INFO)

import reactor_client as r
from reactor_client.reactor_objects import ReactionStatusUpdate, StatusUpdate, \
    OperationIdentifier, OperationStatus
from cloud.dwh.nirvana.reactor import REACTOR_BASE_URL


class ReactorWrapper:
    DEACTIVATION_CHECK_PERIOD = 1  # 1 second

    def __init__(self, base_url=REACTOR_BASE_URL, token=os.environ.get('REACTOR_TOKEN', ''), **kwargs):
        self.client = r.ReactorAPIClientV1(
            base_url=base_url,
            token=token,
            **kwargs
        )

    def replace_reaction(self, builder, local=False):
        if local:
            log.info('Skipping Reaction creation because of local backend')
            return

        client = self.client
        namespace_id = builder.operation_descriptor.namespace_desc.namespace_identifier
        existing_operation_id = OperationIdentifier(namespace_identifier=namespace_id)
        if client.reaction.check_exists(existing_operation_id):
            log.info('Deactivate existing reaction')
            client.reaction.update([ReactionStatusUpdate(existing_operation_id,
                                                         StatusUpdate.DEACTIVATE)])
            while client.reaction.get(existing_operation_id).status != OperationStatus.INACTIVE:
                log.info('Waiting for deactivation...')
                time.sleep(self.DEACTIVATION_CHECK_PERIOD)
            log.info('Deleting existing reaction')
            client.namespace.delete([namespace_id], delete_if_exist=True)
        log.info('Creating new reaction')
        reaction_ref = client.reaction.create(builder.operation_descriptor, create_if_not_exist=True)
        log.info('Created reaction {}/reaction/{}?mode=overview'.format(REACTOR_BASE_URL, reaction_ref.namespace_id))
        operation_id = OperationIdentifier(reaction_ref.reaction_id)
        log.info('Activating new reaction')
        client.reaction.update([ReactionStatusUpdate(operation_id, StatusUpdate.ACTIVATE)])
