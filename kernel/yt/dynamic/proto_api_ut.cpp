#include "proto_api.h"
#include "ut/lib.h"

namespace NYT {
namespace NProtoApiTest {

using namespace NProtoApi;

void CompileTimeTest() {
    TRowsetBuilder<TZoo> builder{ETableSchemaKind::Lookup};
    builder.Add(TZoo());
    builder.AddRef(TZoo());
    builder.AddPartial(TZoo());

    NApi::IClientPtr client;
    IInvokerPtr invoker;

    TZoo in1;
    TVector<TZoo> in2;
    TRange<TZoo> in3;

    LookupRowsAsync<TZoo>(client, "", in1);
    LookupRowsAsync<TZoo>(client, "", in2);
    LookupRowsAsync<TZoo>(client, "", in3);
    LookupRowAsync<TZoo>(client, "", in1);

    LookupRows<TZoo>(client, "", in1);
    LookupRows<TZoo>(client, "", in2);
    LookupRows<TZoo>(client, "", in3);
    LookupRow<TZoo>(client, "", in1);

    LookupRowsVia<TZoo>(invoker, client, "", in1);
    LookupRowsVia<TZoo>(invoker, client, "", in2);
    LookupRowsVia<TZoo>(invoker, client, "", in3);
    LookupRowVia<TZoo>(invoker, client, "", in1);

    SelectRowsAsync<TZoo>(client, "");
    SelectRows<TZoo>(client, "");
    SelectRowsVia<TZoo>(invoker, client, "");

    NApi::ITransactionPtr tx;
    WriteRows<TZoo>(tx, "", in1);
    WriteRows<TZoo>(tx, "", in2);
    WriteRows<TZoo>(tx, "", in3);

    DeleteRows<TZoo>(tx, "", in1);
    DeleteRows<TZoo>(tx, "", in2);
    DeleteRows<TZoo>(tx, "", in3);
}

} // NProtoApViaiTest
} // NYT
