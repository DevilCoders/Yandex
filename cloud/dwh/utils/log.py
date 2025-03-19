import logging
from typing import Tuple
from typing import Union

from colorlog import ColoredFormatter

_MASKED_FIELDS = ('oauth_token', 'oauthToken', 'token', 'iam_token', 'iamToken', 'jwt', 'yandexPassportOauthToken', 'metadata', 'result')
_MASKING_VALUE = '[masked value]'


def setup_logging(debug: bool = False, colored: bool = False):
    max_name_len = 50

    orig_factory = logging.getLogRecordFactory()

    def record_factory(*args, **kwargs):
        record = orig_factory(*args, **kwargs)
        record.fname = f'...{record.name[-max_name_len + 3:]}' if len(record.name) > max_name_len else record.name
        return record

    logging.setLogRecordFactory(record_factory)

    logging.addLevelName(logging.DEBUG, 'D')
    logging.addLevelName(logging.INFO, 'I')
    logging.addLevelName(logging.WARNING, 'W')
    logging.addLevelName(logging.ERROR, 'E')
    logging.addLevelName(logging.CRITICAL, 'C')

    format_ = ''
    format_ += '%(asctime)s.%(msecs)03d '
    format_ += f'[%(fname){max_name_len}.{max_name_len}s:%(lineno)04d][%(levelname)s]: '
    format_ += '%(message)s'
    date_format = '%Y.%m.%d %H:%M:%S'

    if colored:
        formatter = ColoredFormatter('%(log_color)s' + format_, date_format, log_colors={
            logging.getLevelName(logging.DEBUG): 'white',
            logging.getLevelName(logging.INFO): 'green',
            logging.getLevelName(logging.WARNING): 'yellow',
            logging.getLevelName(logging.ERROR): 'red',
            logging.getLevelName(logging.CRITICAL): 'bold_red',
        })
    else:
        formatter = logging.Formatter(format_, date_format)

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG if debug else logging.INFO)
    root_logger.addHandler(handler)


def mask_sensitive_fields(data: dict, extra_fields: Union[str, Tuple[str]] = None) -> dict:
    """Masking fields. Note: nested fields are not masked."""

    if not data or not isinstance(data, dict):
        return data

    result = data.copy()

    fields_to_mask = _MASKED_FIELDS
    if extra_fields is not None:
        fields_to_mask += (extra_fields,) if isinstance(extra_fields, str) else tuple(extra_fields)

    for field in fields_to_mask:
        if field in result:
            result[field] = _MASKING_VALUE

    return result
