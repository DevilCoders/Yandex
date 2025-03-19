**Как запровиженить танки и таргеты:**
* проверить корректность inventory файла
* провижен танков:
```ansible-playbook -i inventory install_tank.yml```
* провижен таргетов:
```ansible-playbook -i inventory install_target.yml```

**Как стрелять**
* патроны лежат в:
```roles/tanks/roles/tank/files/load-config-*```
* проверить что указаны корректные fqdn, ipv6 адреса для таргетов, а также профиль нагрузки
* зайти на танк overlay или underlay:
* запустить стрельбу:
```yandex-tank -c load-config-locallb```
