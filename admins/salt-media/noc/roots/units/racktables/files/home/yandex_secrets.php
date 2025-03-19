{%- macro check_secret_name(name) %}
    {%- if not  name is match('^[a-zA-Z_][a-zA-Z0-9_]*$') %}
    {{- raise('BAD SECRET NAME {!r}, it violates the rules for naming variables in php'.format(name)) }}
    {%- endif %}
{%- endmacro -%}
<?php
/* Yandex Racktables secrets file, generated from yav by salt */
{% for (k, v) in pillar['sec_rt-yandex'].items() %}{{check_secret_name(k)}}
{%- if k == 'yandex_terminal_password' %}
$yandex_terminal_password = '{{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(" ")[0] }}';
$yandex_terminal_passwords = ['{{ "', '".join(pillar["sec_rt-yandex"]["yandex_terminal_password"].split(" ")) }}'];{% else %}
${{ k }} = '{{ v }}';{% endif %}{% endfor %}
$yandex_db_password = array (
        'master' => array (
                'db_username' => 'racktables',
                'db_password' => '{{ pillar["sec_rt-yandex"]["db_user_racktables"] }}',
        ),
        'backup' => array (
                'db_username' => 'racktables_ro',
                'db_password' => '{{ pillar["sec_rt-yandex"]["db_user_racktables_ro"] }}',
        ),
);
$yandex_db_password['ro_on_master'] = $yandex_db_password['backup'];
$yandex_tokens = array
(
    # pwgen -s 24
{% for (k, v) in pillar['sec_rt-yandex-tokens'].items() %}{{check_secret_name(k)}}
   '{{ k }}' => '{{ v }}',{% endfor %}
);
