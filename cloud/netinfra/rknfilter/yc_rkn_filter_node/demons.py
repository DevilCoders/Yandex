#! /usr/bin/env python3


from .bird_configuration import apply_bird_configuration_changes
from .suricata_configuration import apply_suricata_configuration_changes


def apply_new_configuration(logger=None):
    # randtime = random.randrange(0,180)
    # if rand:
    #    logger.info("sleep for {}s".format(randtime))
    #    time.sleep(randtime)
    if logger:
        logger.info(
            "Applying configuration changes for filter node's backend mechanism's"
        )
    apply_bird_configuration_changes(logger=logger)
    apply_suricata_configuration_changes(logger=logger)
    if logger:
        logger.info(
            "Configuration changes for filter node backend is succesfully applied"
        )
