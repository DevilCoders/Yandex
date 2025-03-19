Библиотека предоставляет базовые классы для создания HTTP-сервера. Библиотека используюет идиому NVI, при этом в проcтейшем случае для создания простейшего вебсервера требуется унаследовать два класса и переопределить им по методу (см. пример ниже). В общем случае пользователи могут при необходимости переопределять основные аспекты поведения вебсервера, например, обработку заголовков, создание thread specific ресурсов и др. Основные фичи:
  - Головановская стат-ручка из коробки
  - Поддержка аппхостового интерфейса
  - Поддержка ITS
  - Таймауты на запросы
  - Админские ручки из коробки

В целом библиотека хорошо подходит для создания вебсервера предназначенного для развертывания в поисковом облаке, но может быть использована для написания серверов, разворачиваемых и за его пределами.

# Пример использования
```cpp
#include <kernel/server/server.h>
#include <kernel/server/protos/serverconf.pb.h>

// Создаем свой класс-обработчик запросов, наследуясь от TRequest
class TTestRequest : public NServer::TRequest {
public:
    explicit TTestRequest(NServer::TServer& server)
        : NServer::TRequest{server} {
    }

    // Для примера будем возвращать что-нибудь бесполезное по пути /hello
    bool DoReply(const TString& script, THttpResponse& response) override {
        if (script == "/hello") {
            response = TextResponse("Hello, user!");
            return true;
        }

        return false;
    }
};

// Создаем свой класс сервера, наследуясь от TServer
class TTestServer : public NServer::TServer {
public:
    explicit TTestServer(const NServer::THttpServerConfig& config)
        : NServer::TServer{config} {
    }

    TClientRequest* CreateClient() override {
        // Будем возвращать наш унаследованный обработчик запросов (см. выше)
        return new TTestRequest(*this);
    }
};

int main() {
    // Протобуфный конфиг, будем использовать все параметры по умолчанию
    const NServer::THttpServerConfig config;

    // Инициализируем и запускаем сервер
    TTestServer server{config};
    server.Start();
    server.Wait(); // отсюда сервер выйдет если будет выключен через админскую ручку

    return 0;
}
```

Собираем и пробуем, 17000 - дефолтный порт из протобуфного конфига, наша ручка:
```
> http "localhost:17000/hello"
HTTP/1.1 200 Ok
Connection: Keep-Alive
Content-Length: 9
Content-Type: text/plain
Date: Mon, 04 Dec 2017 14:23:33 GMT
Timing-Allow-Origin: *

Sup, dog?
```

Головановская стат-ручка из коробки:
  - гистограммы времен ответов
  - гистограммы HTTP-кодов
  - гистограммы размеров внутренних очередей
```
> http 'localhost:17000/remote_admin?action=mstat'
HTTP/1.1 200 Ok
Connection: Keep-Alive
Content-Length: 1087

[["answer_time_dhhh",[[0,8],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],[9,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,0],[80,0],[90,0],[100,0],[150,0],[200,0],[250,0],[300,0],[350,0],[400,0],[450,0],[500,0],[1000,0],[5000,0]]],["production_answer_time_dhhh",[[0,6],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],[9,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,0],[80,0],[90,0],[100,0],[150,0],[200,0],[250,0],[300,0],[350,0],[400,0],[450,0],[500,0],[1000,0],[5000,0]]],["codes_200_dmmv",6],["codes_404_dmmv",1],["codes_other_dmmv",1],["request_queue_size_dhhh",[[0,9],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],[9,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,0],[80,0],[90,0],[100,0],[200,0],[300,0],[400,0],[500,0],[1000,0],[2000,0],[5000,0],[10000,0],[20000,0]]],["request_queue_reject_dmmm",0],["fail_queue_size_dhhh",[[0,9],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],[9,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,0],[80,0],[90,0],[100,0],[200,0],[300,0],[400,0],[500,0],[1000,0],[2000,0],[5000,0],[10000,0],[20000,0]]],["fail_queue_reject_dmmm",0]]
```

Админские ручки из коробки:
```
> http 'localhost:17000/admin?action=shutdown'
HTTP/1.1 200 Ok
Connection: Keep-Alive
Content-Length: 14
Content-Type: text/plain
Date: Mon, 04 Dec 2017 14:37:09 GMT
Timing-Allow-Origin: *

shutting down
```
# Примечания
  - Стат-ручка появляется из коробки, но для того чтобы сигналы появились в головане - нужено научить агент голована ходить в эту ручку, подробности в [документации Голована](https://wiki.yandex-team.ru/golovan)
  - [Шаблон панели](https://yasm.yandex-team.ru/template/panel/kernel_server/server_name=informers;itype=pdbinformers;prj=pdb/) (с параметрами для информеров Коллекций)

# См. также
  - [Голован](https://wiki.yandex-team.ru/golovan)
  - [AppHost](https://wiki.yandex-team.ru/serp/development/apphost/)


