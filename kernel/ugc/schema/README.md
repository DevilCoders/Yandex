# Схема для UGCDB

Схема позволяет описать логическую структуру данных, а затем по этому описанию сгенерировать набор protobuf и cpp файлов для работы с физическими данными в рамках описанной структуры. Описание схемы задается на неком DDL (data definition language), который призван максимально выразительно отражать предметную область. Сгенерированные по схеме файлы собираются в библиотеки, которые можно использовать в прикладном коде как обычно.

## Сгенерированный API

Допустим, есть такое описание схемы:

```
SCHEMA Ugc {
  TABLE User {
      KEY UserId;
      TABLE Feedback {
          KEY FeedbackId;
          ROW TUserFeedback {
              float Rating;
              string Review;
          }
      }
  }
}
```

Для него будет сгенерированы следующие структуры и функции.

### Protobuf

Строка в таблице:

```
message TUserFeedback {
    optional string UserId = 1;
    optional string FeedbackId = 2;
    optional float Rating = 3;
    optional string Review = 4;
}
```

Специальный protobuf-контейнер, позволяющий работать с любой таблицей из схемы, не зная ее имени:

```
message TUgcEntity {
    oneof Row {
        TUserFeedback UserFeedback = 1;
    }
}
```

Protobuf для описания апдейта в старом формате ugcupdate:

```
message TUserFeedbackUpdateRow {
    optional string Key = 1;
    optional string FeedbackId = 2;
    optional float Rating = 3;
    optional string Review = 4;
}

message TUserUpdateProto {
    optional string Type = 1;
    optional string Version = 2;
    optional string App = 3;
    optional uint64 Time = 4;
    optional string UpdateId = 5;
    optional string UserId = 6;
    optional string VisitorId = 7;
    optional string DeviceId = 8;
    repeated TUserFeedbackUpdateRow Feedback = 9;
}
```

### C++

Получить указатель на иницилизированную строку таблицы:

```
google::protobuf::Message* GetEntityRow(TUgcEntity& entity)
```

Создать протобуф-контейнер из пути к объекту вида `/user/123/feedback/abc`:

```
TUgcEntity CreateUgcEntity(TStringBuf path)
```

Создать апдейт в старом формате ugcupdate из строки таблицы:

```
TUserUpdateProto CreateUserUpdateProto(const TUserFeedback& row)
```

Создать апдейт из json:

```
TUserUpdateProto JsonToUserUpdateProto(const TString& s)
```

Конвертировать protobuf-апдейт в json:

```
TString UserUpdateProtoToJson(const TUserUpdateProto& update)
```
