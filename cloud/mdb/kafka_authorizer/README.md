**Yandex.Cloud Kafka authorizer**

Currently provides readonly mode through blocking write requests
 when Kafka logs partition reaches 97% of storage space.

Authorizer is inherited from default authorizer ```kafka.security.auth.SimpleAclAuthorizer```
 and calls its ```authorize()``` first  so it should be compatible

**Build**
https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_KafkaAuthorizer

**Installation**

tldr:
```apt install kafka-authorizer```

details:
```kafka_authorizer.jar``` must be placed to ```/opt/kafka/libs/```
and ```authorizer.class.name``` property in ```/etc/kafka/server.properties``` must be set to ```com.yandex.cloud.mdb.kafka.YandexCloudAuthorizer```
then Kafka must restart ```systemctl restart kafka.service```

