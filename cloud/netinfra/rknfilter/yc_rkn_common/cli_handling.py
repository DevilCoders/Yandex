import json

from .custom_exceptions import input_error_exception
from .manage_configuration_files import read_content_of_suplementary_files


def enrich_cli_input(args, test_info=dict(), logger=None):
    args_dict = vars(args)
    logger.debug("Parsing CLI input")
    # converting cli input argeuments JSONs to dicts
    for argument in [
        "dump_files_locations",
        "config_files_locations",
        "whitelist_asns_files_locations",
        "friendly_nets_files_locations",
    ]:
        if argument in args_dict and args_dict[argument]:
            args_dict[argument] = json.loads(args_dict[argument])
    if None in list(args_dict.values()):
        logger.debug("CLI input is not complete. Checking alternaive sources")
        populate_with_info_from_periferial_sources(args_dict, test_info, logger=logger)
    else:
        logger.debug("CLI input is complete.")
    logger.info("Proccessing of input data is finished")
    return args_dict


def populate_with_info_from_periferial_sources(
    args_dict, test_info=dict(), logger=None
):
    digged_parameters = dict()
    configuration_files_locations = {args_dict["config_file"], args_dict["auth_file"]}
    parameters_fetched_from_files = read_content_of_suplementary_files(
        configuration_files_locations, logger=logger
    )
    logger.debug("Reading contents of utility configuration files is finished")
    for parameter, value in args_dict.items():
        if value is None:
            logger.debug("Value for parameter is not set: {}".format(parameter))
            if parameter in parameters_fetched_from_files:
                logger.debug("Trying to fetch it from configuration file")
                digged_parameters[parameter] = parameters_fetched_from_files[parameter]
            elif parameter in test_info:
                logger.debug("Trying to fetch it from debugging constants")
                digged_parameters[parameter] = test_info[parameter]
            else:
                raise input_error_exception(
                    "Could not find value for mandatory parameter: {}. "
                    "Plz check your inputs".format(parameter)
                )
            logger.debug(
                "Value for parameter is succesfully fetched: {}".format(parameter)
            )
        else:
            logger.debug("Value for parameter is already set: {}".format(parameter))
    args_dict.update(digged_parameters)
