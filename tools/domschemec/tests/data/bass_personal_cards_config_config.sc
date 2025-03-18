namespace NConfig;

struct TConfig {
    HttpPort (required) : ui16;
    HttpThreads : ui16 (default = 10);
    UserUpdateThreads : ui16 (default = 1);
    UserUpdateMaxRPS : ui16 (default = 5); // max update rps per-thread
    MongoMasterURI (required) : string;
    MongoReplicaURI : string;

    // test specific stuff
    WarmupMongoConnections: bool (default = true);
    TestUsersMongoURI : string (default = "mongodb://man1-5926.search.yandex.net:30120,sas1-5926.search.yandex.net:9640,vla1-2875.search.yandex.net:17660/?replicaSet=personal_cards");
};
