import logging
import logging.handlers
from concurrent.futures import ThreadPoolExecutor


def create_logger_object(log_file, logger_name, quiet, verbose_output):
    logger = logging.getLogger(logger_name)
    file_handler = logging.handlers.RotatingFileHandler(
        log_file, maxBytes=10000000, backupCount=3
    )
    formatter = logging.Formatter(
        "%(asctime)s %(filename)s[LINE:%(lineno)d]# %(levelname)-8s %(message)s",
        "%Y-%m-%d %H:%M:%S",
    )
    file_handler.setFormatter(formatter)
    if not quiet:
        stream_handler = logging.StreamHandler()
        stream_handler.setFormatter(formatter)
        logger.addHandler(stream_handler)
    if verbose_output:
        logger.setLevel(logging.DEBUG)
        logger.debug("Enabled verbose mode")
    else:
        logger.setLevel(logging.INFO)
    logger.addHandler(file_handler)
    return logger


def run_in_parallel(function, element_range, limit=256, logger=None, **kwargs):
    amount_of_records = len(element_range)
    percentage = int(amount_of_records / 100) + 1
    if percentage == 0:
        percentage = 1
    with ThreadPoolExecutor(max_workers=limit) as executor:
        #        [
        #            executor.submit(function, element, **kwargs)
        #            for element in  element_range
        #        ]
        for element_order, element in enumerate(element_range, 1):
            if element_order % percentage == 0:
                if logger:
                    logger.debug(
                        "{} records from {} are processed ({}%)".format(
                            element_order,
                            amount_of_records,
                            int(100 * element_order / amount_of_records),
                        )
                    )
            executor.submit(function, element, **kwargs)
    if logger:
        logger.debug("all records are succesfully processed")
