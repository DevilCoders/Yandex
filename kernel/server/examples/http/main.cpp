#include <kernel/server/protos/serverconf.pb.h>
#include <kernel/server/server.h>

#include <util/system/info.h>

// Создаем свой класс-обработчик запросов, наследуясь от TRequest
class TTestRequest final : public NServer::TRequest {
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
class TTestServer final : public NServer::TServer {
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
    NServer::THttpServerConfig config;
    config.SetThreads(NSystemInfo::CachedNumberOfCpus());

    // Инициализируем и запускаем сервер
    TTestServer server{config};
    server.Start();
    server.Wait(); // отсюда сервер выйдет если будет выключен через админскую ручку

    return 0;
}
