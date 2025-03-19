yav: {{salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n')|json}}
yavape: {{salt.yav.get('sec-01g0f6jn51ddzby5rcskzqs31k')|json}}
music-secrets: {{salt.yav.get('sec-01cwrw7ed7jkc75n2dyrhvp0ca')|json}}
yarl-secrets: {{salt.yav.get('sec-01db0bze5j8p3kyannxaabtmmc')|json}}
valve_password: {{salt.yav.get('sec-01eahny2hcegfcyeytwk7kadqx')|json}}
valve_pg: {{salt.yav.get('sec-01dpbgtxvx63gee04ndq5n6xd8')|json}}
valve_s3: {{salt.yav.get('sec-01dpbh06hmmmn2gzcz3dhj2wvf')|json}}
valve_oauth: {{salt.yav.get('sec-01dtp89db537cmsa9j59zhc8vs')|json}}
valve_secrets: {{salt.yav.get('sec-01ewdwywfgbc08e3zejj7shj7t')|json}}
stat-reporter-tvm-secret: {{ salt.yav.get('sec-01ebgs2cjekrcd0h835agfg40h[client_secret]') | json }}
s3_tvm_secret_testing: {{ salt.yav.get('sec-01dwerr2nefd70qpsgbr850cv6[client_secret]') | json }}
s3_tvm_secret_prod: {{ salt.yav.get('sec-01dwerrcfxesrr89z1jqht3169[client_secret]') | json }}
s3_logbroker_oauth: "{{ salt.yav.get('sec-01ekd5btq77e1nza5zy7hs8ne8[oauth_token]') }}"

robot_storage_duty_yav: {{ salt.yav.get('sec-01ejz19avjqkkey7ggpkkztrhp') | json }}

{% if grains['yandex-environment'] == 'testing' -%}
tls_elliptics: {{salt.yav.get('sec-01dz16mcbhkbhhc3zbrfc7xnr7')|json}}
tls_karl: {{salt.yav.get('sec-01efkq40eze0j6ac96d0bjjp9d')|json}}
mediastorage_proxy_auth_sign_token: {{salt.yav.get('sec-01fzdqwt007k7w67qnq4wrvqc5')|json}}
s3_v1_cipher_key: "{{ salt.yav.get('sec-01fzft0hhy8zv38enbax5rqckk[v1_cipher_key-testing_base64]') }}"
{%- else -%}
tls_elliptics: {{salt.yav.get('sec-01dz16gr5cpgzv5hh5ya7gmz8k')|json}}
tls_karl: {{salt.yav.get('sec-01efkqdwhc1q1aj4ga53cp0r37')|json}}
mediastorage_proxy_auth_sign_token: {{salt.yav.get('sec-01fzdqyyektfxkqz0t0kgz5bt9')|json}}
s3_v1_cipher_key: "{{ salt.yav.get('sec-01fzft0hhy8zv38enbax5rqckk[v1_cipher_key_base64]') }}"
{%- endif %}
