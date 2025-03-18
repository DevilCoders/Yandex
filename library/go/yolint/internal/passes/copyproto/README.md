# Motivation

Starting from [protobuf-apiv2](https://blog.golang.org/protobuf-apiv2) it is illigal to use shadow copy for proto messages,
for more info see here: [arcadia-announce](https://clubs.at.yandex-team.ru/arcadia/22420) , and here https://github.com/golang/protobuf/issues/1079

Please use [proto.Clone](https://pkg.go.dev/github.com/golang/protobuf/proto?tab=doc#Clone) instead.

## Links
- Original [post](https://clubs.at.yandex-team.ru/arcadia/22420)
- Original tiket [IGNIETFERRO-1453](https://st.yandex-team.ru/IGNIETFERRO-1453) 
