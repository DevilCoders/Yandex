#include "wrappers.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/scope.h>

namespace NMongo {
    TReadPreferences::TReadPreferences() {
    }

    TReadPreferences::TReadPreferences(mongoc_read_mode_t mode) {
        EnsureCreated(mode);
    }

    TReadPreferences::TReadPreferences(TReadPreferences&& other) noexcept {
        std::swap(Value, other.Value);
    }

    inline void TReadPreferences::EnsureCreated(mongoc_read_mode_t mode) {
        if (!Value) {
            Value = mongoc_read_prefs_new(mode);
            Y_ENSURE(Value != nullptr, "Couldn't create mongo read prefs");
        }
    }

    void TReadPreferences::SetMode(mongoc_read_mode_t mode) {
        EnsureCreated(mode);
        mongoc_read_prefs_set_mode(Value, mode);
    }

    void TReadPreferences::SetTags(const TBsonValue& tags) {
        EnsureCreated(MONGOC_READ_PRIMARY_PREFERRED);
        mongoc_read_prefs_set_tags(Value, tags);
    }

    bool TReadPreferences::Validate() const {
        return !Value || mongoc_read_prefs_is_valid(Value);
    }

    TReadPreferences::~TReadPreferences() {
        if (Value) {
            mongoc_read_prefs_destroy(Value);
        }
    }

    TUri::TUri(const TString& connectionString)
        : TMongoStructureHolder(mongoc_uri_new(connectionString.data()))
    {
    }

    TUri::~TUri() {
        mongoc_uri_destroy(*this);
    }

    TClient::TClient(const TClientPool& pool)
        : TMongoStructureHolder(mongoc_client_pool_pop(pool))
        , Pool(pool)
    {
    }

    TClient::~TClient() {
        mongoc_client_pool_push(Pool, *this);
    }

    TClientPool::TClientPool(const TUri& uri)
        : TMongoStructureHolder(mongoc_client_pool_new(uri))
    {
    }

    TClientPool::~TClientPool() {
        mongoc_client_pool_destroy(*this);
    }

    TDatabase::TDatabase(const TClient& client, const TString& db)
        : TMongoStructureHolder(mongoc_client_get_database(client, db.data()))
    {
    }

    TDatabase::~TDatabase() {
        mongoc_database_destroy(*this);
    }

    TCollection::TCollection(const TClient& client, const TString& db,
                             const TString& collection)
        : TMongoStructureHolder(mongoc_client_get_collection(client, db.data(), collection.data()))
    {
    }

    TCollection::~TCollection() {
        mongoc_collection_destroy(*this);
    }

    TCursor::TCursor(mongoc_cursor_t* value)
        : TMongoStructureHolder(value)
    {
    }

    TCursor::~TCursor() noexcept(false) {
        bson_error_t error;
        if (mongoc_cursor_error(operator mongoc_cursor_t*(), &error)) {
            mongoc_cursor_destroy(*this);
            ythrow TMongoException{error};
        }
        mongoc_cursor_destroy(*this);
    }

    TMaybe<TBsonValue> TCursor::Begin() {
        return Next();
    }

    TMaybe<TBsonValue> TCursor::Next() {
        const bson_t* doc;
        if (mongoc_cursor_next(*this, &doc)) {
            return TBsonValue(doc);
        } else {
            return TMaybe<TBsonValue>();
        }
    }

    TBulkOperation::TBulkOperation(mongoc_bulk_operation_t* value)
        : TMongoStructureHolder(value)
    {
    }

    TBulkOperation::~TBulkOperation() noexcept(false) {
        mongoc_bulk_operation_destroy(*this);
    }

    TBsonValue::TBsonValue()
        : TBsonValue(NJson::TJsonValue(NJson::JSON_MAP))
    {
    }

    TBsonValue::TBsonValue(bson_t* value, bool shouldCopy)
        : Value(shouldCopy ? bson_copy(value) : value)
    {
    }

    TBsonValue::TBsonValue(const bson_t* value)
        : Value(bson_copy(value))
    {
    }

    TBsonValue::TBsonValue(const NJson::TJsonValue& json)
        : Value(bson_new_from_json(reinterpret_cast<const ui8*>(NJson::WriteJson(json, false).data()), -1, nullptr))
    {
        Y_ENSURE(Value != nullptr, "Couldn't create bson value from " << NJson::WriteJson(json, false));
    }

    TBsonValue::TBsonValue(const TString& json)
        : Value(bson_new_from_json(reinterpret_cast<const ui8*>(json.data()), json.length(), nullptr))
    {
        Y_ENSURE(Value != nullptr, "Couldn't create bson value from " << json);
    }

    TBsonValue::TBsonValue(const TBsonValue& other)
        : Value(bson_copy(other))
    {
    }

    TBsonValue::~TBsonValue() {
        Destroy();
    }

    TBsonValue& TBsonValue::operator=(const TBsonValue& other) {
        Destroy();
        Value = bson_copy(other.Value);
        return *this;
    }

    void TBsonValue::Destroy() {
        if (Value != nullptr) {
            bson_destroy(Value);
            Value = nullptr;
        }
    }

    bson_t* TBsonValue::Release() {
        auto result = Value;
        Value = nullptr;
        return result;
    }

    NJson::TJsonValue TBsonValue::ToJson() const {
        NJson::TJsonValue result;
        auto bsonStr = bson_as_json(Value, nullptr);
        NJson::ReadJsonFastTree(bsonStr, &result);
        bson_free(bsonStr);
        return result;
    }

    THelper::THelper(const TString& connectionString)
        : ClientPool(connectionString)
    {
    }

    bool THelper::Insert(const TString& db, const TString& collectionName,
                         const TBsonValue& value, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        bool result = mongoc_collection_insert(collection, MONGOC_INSERT_NONE,
                                               value, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::Update(const TString& db, const TString& collectionName,
                         const TBsonValue& selector, const TBsonValue& updater, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        bool result = mongoc_collection_update(collection, MONGOC_UPDATE_NONE,
                                               selector, updater, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::Upsert(const TString& db, const TString& collectionName,
                         const TBsonValue& selector, const TBsonValue& updater, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        bool result = mongoc_collection_update(collection, MONGOC_UPDATE_UPSERT,
                                               selector, updater, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::MultiUpdate(const TString& db, const TString& collectionName,
                              const TBsonValue& selector, const TBsonValue& updater, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        bool result = mongoc_collection_update(collection, MONGOC_UPDATE_MULTI_UPDATE,
                                               selector, updater, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::MultiUpsert(const TString& db, const TString& collectionName,
                              const TBsonValue& selector, const TBsonValue& updater, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        constexpr auto MULTI_UPSERT = static_cast<mongoc_update_flags_t>(MONGOC_UPDATE_UPSERT | MONGOC_UPDATE_MULTI_UPDATE);
        bool result = mongoc_collection_update(collection, MULTI_UPSERT,
                                               selector, updater, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::BulkInsert(const TString& db, const TString& collectionName,
                    const TVector<TBsonValue>& values, bool ordered, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        TBulkOperation bulk(mongoc_collection_create_bulk_operation(collection, ordered, nullptr));
        for (const auto& value : values) {
            mongoc_bulk_operation_insert(bulk, value);
        }
        bson_t reply;
        bson_error_t err;
        bool result = mongoc_bulk_operation_execute(bulk, &reply, &err) != 0;
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::BulkUpdate(const TString& db, const TString& collectionName,
                             const TVector<TUpdateParams>& ops, bool ordered, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        TBulkOperation bulk(mongoc_collection_create_bulk_operation(collection, ordered, nullptr));
        for (const auto& op : ops) {
            mongoc_bulk_operation_update(bulk, op.Selector, op.Updater, op.Upsert);
        }
        bson_t reply;
        bson_error_t err;
        bool result = mongoc_bulk_operation_execute(bulk, &reply, &err) != 0;
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    bool THelper::Remove(const TString& db, const TString& collectionName,
                         const TBsonValue& selector, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        bool result = mongoc_collection_remove(collection, MONGOC_REMOVE_NONE,
                                               selector, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    void THelper::Find(const TString& db, const TString& collectionName,
                       const std::function<void(const TBsonValue&)>& callback,
                       const TBsonValue& selector, size_t skip, size_t limit, const TBsonValue& fields,
                       const TReadPreferences& readPrefs) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);

        TCursor cursor(mongoc_collection_find(collection, MONGOC_QUERY_NONE,
                                              skip, limit, 0, selector, fields, readPrefs));
        for (auto it = cursor.Begin(); !it.Empty(); it = cursor.Next()) {
            callback(*it);
        }
    }

    TVector<TBsonValue> THelper::Find(const TString& db, const TString& collectionName,
                                      const TBsonValue& selector, size_t skip, size_t limit, const TBsonValue& fields,
                                      const TReadPreferences& readPrefs) {
        TVector<TBsonValue> result;
        Find(db, collectionName,
             [&result](const TBsonValue& value) {
                 result.push_back(value);
             },
             selector, skip, limit, fields, readPrefs);
        return result;
    }

    TBsonValue THelper::FindAndModify(const TString& db, const TString& collectionName,
                                      const TBsonValue& query, const TBsonValue& update,
                                      const TBsonValue& sort, bool upsert, bool retnew,
                                      bool remove, const TBsonValue& fields, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);

        // Use raw bson_t here because mongoc_collection_find_and_modify expects uninitialized
        // bson_t object and it won't destroy previous object there.
        bson_t result;
        bson_error_t err;
        if (!mongoc_collection_find_and_modify(collection, query, sort, update,
                                               fields, remove, upsert, retnew, &result, &err))
        {
            if (error != nullptr) {
                *error = err;
            }
            return TBsonValue(nullptr, false);
        }
        TBsonValue ret(&result, true);
        bson_destroy(&result);
        return ret;
    }

    i64 THelper::Count(const TString& db, const TString& collectionName,
                       const TBsonValue& selector, size_t skip, size_t limit, TError* error) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);
        bson_error_t err;
        i64 result = mongoc_collection_count(collection, MONGOC_QUERY_NONE, selector,
                                             skip, limit, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    void THelper::Aggregate(const TString& db, const TString& collectionName,
                            const std::function<void(const TBsonValue&)>& callback,
                            const TBsonValue& pipeline,
                            const mongoc_query_flags_t flags) {
        TClient client(ClientPool);
        TCollection collection(client, db, collectionName);

        TCursor cursor(mongoc_collection_aggregate(collection, flags,
                                                   pipeline, nullptr, nullptr));
        for (auto it = cursor.Begin(); !it.Empty(); it = cursor.Next()) {
            callback(*it);
        }
    }

    TVector<TBsonValue> THelper::Aggregate(const TString& db,
                                           const TString& collectionName, const TBsonValue& pipeline,
                                           const mongoc_query_flags_t flags) {
        TVector<TBsonValue> result;
        Aggregate(db, collectionName,
                  [&result](const TBsonValue& value) {
                      result.push_back(value);
                  },
                  pipeline, flags);
        return result;
    }
    TClient THelper::Connect() {
        return TClient(ClientPool);
    }

    bool THelper::CheckConnection(const TString& db, TError* error) {
        NJson::TJsonValue ping;
        ping["ping"] = 1;
        TBsonValue command(ping);

        TClient client(ClientPool);
        bson_error_t err;
        bool result = mongoc_client_command_simple(client, db.data(), command, nullptr, nullptr, &err);
        if (error != nullptr) {
            *error = err;
        }
        return result;
    }

    double THelper::WarmUp(const TString& db, int retryCount) {
        NJson::TJsonValue ping;
        ping["ping"] = 1;
        TBsonValue command(ping);

        int total = 0;
        int success = 0;

        TVector<mongoc_client_t*> clients;

        Y_DEFER {
            for (mongoc_client_t* client : clients) {
                mongoc_client_pool_push(ClientPool, client);
            }
        };

        mongoc_client_t* client;

        while ((client = mongoc_client_pool_try_pop(ClientPool)) != nullptr) {
            clients.push_back(client);
            bson_error_t error;
            int attempts = retryCount;
            while (attempts--) {
                if (mongoc_client_command_simple(client, db.data(), command, nullptr, nullptr, &error)) {
                    ++success;
                    break;
                }
            }
            ++total;
        }

        return 1.0 * success / total;
    }
}
