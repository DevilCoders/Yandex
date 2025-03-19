PASSPORT = {
    'oauth': {
        'tokens': {
{% for data in salt['pillar.get']('data:blackbox-mock:tokens') %}
            "{{ data['token'] }}": {
                'login': "{{ data['login'] }}",
                'scope': "{{ data['scope'] }}",
                'status': 'VALID',
            },
{% endfor %}
        },
    },
}
