{% note alert %}

Эта функциональность в разработке https://st.yandex-team.ru/CLOUD-46216

{% endnote %}

Бывают ситуации, когда сервис не возвращает ресурсов, но по каким-либо причинам вызовы этого сервиса необходимо
авторизовывать. В этом случае можно выдавать права на специальный ресурс с идентификатором `gizmo` и типом ресурса
`iam.gizmo`.

```proto
service GizmoService {
  rpc ListAccessBindings (ListAccessBindingsRequest) returns (ListAccessBindingsResponse)
  rpc UpdateAccessBindings (UpdateAccessBindingsRequest) returns (UpdateAccessBindingsResponse)
}
```

Например, такая ситуация возникает в методе сервиса, отдающем статистику по работе этого сервиса, где в контексте
авторизации проблематично выделить какой-либо ресурс.

```proto
service StatisticsService {
  rpc Get (GetStatisticsRequest) returns (GetStatisticsResponse)
}
```

{% note alert %}

Данный метод авторизации применяется, только если ресурс действительно нельзя выделить. В большинстве случаев
предпочтительнее завести новый тип ресурса в сервисе, разместить экземпляры этого типа в каталогах сервисного облака и
авторизовывать их.

{% endnote %}

{% note info %}

Выдавать права на этот тип ресурса можно только через синк-сервис.

{% endnote %}
