# https://st.yandex-team.ru/TRAFFIC-10036
/etc/network/if-up.d/antiblock:
  file.managed:
    - source: salt://{{slspath}}/files/antiblock
    - mode: 0755
