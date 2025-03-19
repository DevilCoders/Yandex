{% for status in ['NOT_FOUND', 'INVALID_ARGUMENT', 'ALREADY_EXISTS', 'FAILED_PRECONDITION', 'ABORTED', 'DATA_LOSS', 'OUT_OF_RANGE', 'UNAUTHENTICATED', 'PERMISSION_DENIED'] %}
group_lines('sum', { dc='<< dc >>',
 name='<< status|grpc_status_to_signal((grpc_interface|resolve)['prefix'], (grpc_interface|resolve)['postfix']) >>',   host='<< (grpc_interface|resolve)['host'] >>'})
{% if not loop.last %}+{% endif %}
{% endfor %}
