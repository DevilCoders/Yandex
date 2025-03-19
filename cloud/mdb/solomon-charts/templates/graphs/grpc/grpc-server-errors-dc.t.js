{% for status in ['CANCELLED', 'UNKNOWN', 'RESOURCE_EXHAUSTED', 'INTERNAL', 'UNIMPLEMENTED', 'DEADLINE_EXCEEDED', 'UNAVAILABLE'] %}
group_lines('sum', { dc='<< dc >>',
 name='<< status|grpc_status_to_signal((grpc_interface|resolve)['prefix'], (grpc_interface|resolve)['postfix']) >>',   host='<< (grpc_interface|resolve)['host'] >>'})
{% if not loop.last %}+{% endif %}
{% endfor %}
