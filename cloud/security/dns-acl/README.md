# DNS ACL
Здесь лежит последняя версия ACL облачных неймспейсов в slayer dns.
Сделана для удобства управления и ревью изменений.

## Внесение изменений
Для внесения изменений необходимо следующее:
1. Сделать pull-request в acl.json:
 * За редким исключением доступ выдаётся только роботам и ролям в ABC
 * В доступе на abc-сервис всегда должна быть указана роль
 * JSON должен оставаться валидным
2. Призвать /duty dns в ревью

## Выкладка изменений
Выкладка изменений производится /duty dns согласно следующей инструкции:
1. Проверить изменения с прошлой выложенной ревизии - https://a.yandex-team.ru/arc/diff/trunk/arcadia/cloud/security/dns-acl/acl.json?prevRev=rXXXXXX&rev=rYYYYYY  
Обратить внимание на появление новых/удаление старых неймспейсов
2. Создать CLOUDOPS-тикет. Дальнейшие действия проводятся в соответствии с [регламентом на проведение Ad-Hoc операций](https://wiki.yandex-team.ru/cloud/regulations/adhoc/).  
Если изменение затрагивает PROD или PRE-PROD неймспейсы или обратные зоны, то обязательно нужно одобрение на проведение операции от `/duty IM`.
3. Создать новую версию [ACL в yav](https://yav.yandex-team.ru/secret/sec-01cz38q7s2azaeqc4jrcstg5v6/). Описание версии:
```
Ticket - CLOUDOPS-XXXXX
Diff -  https://a.yandex-team.ru/arc/diff/trunk/arcadia/cloud/security/dns-acl/acl.json?prevRev=rXXXXXX&rev=rYYYYYY
Revision - https://a.yandex-team.ru/trunk/arcadia/cloud/security/dns-acl/acl.json?rev=YYYYYY
```
4. В течение 2-5 минут проверить что версия применилась успешно  
```bash
DNS_TOKEN="$(grep -oP '(?<=access_token\>).*?(?=\<\/access_token>)' ~/.dns-monkey/token-oauth.txt)"
curl -H "X-Auth-Token: ${DNS_TOKEN}" -H'Accept: application/json' "https://dns-api.yandex.net/v2.0/${USER}/acl2/status/yandex.cloud"
```
Если не применилась с ошибкой, внести необходимое исправление.
Если не применилось в течение 15 минут и более, проверить json на валидность и написать на dns@yandex-team.ru

