# oauth_token = "{{ pillar['sec_rt-yandex']['py_oauth_1'] }}"
# 2020-05-05
# oauth_token = "{{ pillar['sec_rt-yandex']['py_oauth_2'] }}"
# TEMP HACK TODO!
oauth_token = "{{ pillar['sec_rt-yandex']['py_oauth'] }}"
yandex_terminal_password = "{{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }}"
yandex_terminal_passwords = ["{{ '", "'.join(pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')) }}"]

