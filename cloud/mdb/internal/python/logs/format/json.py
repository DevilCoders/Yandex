import json
import logging


class JsonFormatter(logging.Formatter):
    """
    Format message as JSON.
    """

    OMIT_FIELDS = {
        'created',
        'msecs',
        'relativeCreated',
        'thread',
        'exc_text',
        'exc_info',
        'process',
        'stack_info',
        'args',
    }

    def format(self, record):
        record.asctime = self.formatTime(record, '%Y-%m-%dT%H:%M:%S')

        fields = list(vars(record))

        log_data = {}

        if record.exc_info:
            exc = self.formatException(record.exc_info)
            log_data['stackTrace'] = exc

        for field_name in fields:
            if field_name in self.OMIT_FIELDS:
                continue
            log_data[field_name] = getattr(record, field_name)

        log_data['orig_msg'] = log_data['msg']
        log_data['msg'] = record.getMessage()
        if 'levelname' in log_data:
            log_data['level'] = log_data['levelname']
            del log_data['levelname']

        return json.dumps(log_data)
