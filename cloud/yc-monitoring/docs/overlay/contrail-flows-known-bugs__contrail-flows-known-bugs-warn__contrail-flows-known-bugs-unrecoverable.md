[Алерт contrail-flows-known-bugs в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-flows-known-bugs)

[Алерт contrail-flows-known-bugs-warn в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-flows-known-bugs-warn)

[Алерт contrail-flows-known-bugs-unrecoverable в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-flows-known-bugs-unrecoverable)

## Что проверяет

Наличие известных проблем с установкой flow флоу

- Замечание! У `Invalid NAT flow to private address` есть false postitives: [CLOUD-61266](https://st.yandex-team.ru/CLOUD-61266)

- `contrail-flows-known-bugs-warn` скорее всего не нуждается в починке

Может флапать, поэтому на агрегате включена флаподавилка.

Больше информации про проверку:

- [CLOUD-46838](https://st.yandex-team.ru/CLOUD-46838)

- [flowclass/cls_known_bugs.go](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/opencontrail/pkg/introspect/agent/flowclass/cls_known_bugs.go)

## Если загорелось

- В случае `contrail-flows-known-bugs-warn` - ничего страшного

- В случае `contrail-flows-known-bugs` - дождаться autorecovery и читать логи `sudo journalctl -u vrouter-flows-known-bugs-autorecovery`

- В случае `contrail-flows-known-bugs-unrecoverable`:

  - для [CLOUD-6005](https://st.yandex-team.ru/CLOUD-6005) — `sudo ip link set dev tapxxxx-0 down ; sleep 2 ; sudo ip link set dev tapxxxx-0 up`

  - если не помогает/не подходит `safe-restart --force contrail-vrouter-agent`