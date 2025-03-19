
TIMEOUT_SECONDS = 5
from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import PQStreamingAPI, ConsumerMessageType
from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import ProducerConfigurator, ConsumerConfigurator

if __name__ == "__main__":
    api = PQStreamingAPI("logbroker.yandex.net", 2135)
    f = api.start()
    f.result(TIMEOUT_SECONDS)

    reads_infly = 1
    config = ConsumerConfigurator(
            topic='yacloud-cloudai--yacloud-cloudai-billing', client_id='my_test_reader', max_count=1, use_client_locks=False, read_infly_count=reads_infly
            )
    reader = api.create_consumer(config)
    response = reader.start().result(timeout=TIMEOUT_SECONDS)
    assert response.HasField('Init')

    f = reader.next_event()
    response = f.result(timeout=TIMEOUT_SECONDS)
    assert response.type == ConsumerMessageType.MSG_DATA
    assert response.message.HasField('Data')

    for batch in response.message.Data.MessageBatch:
        for message in batch.Message:
            print message.Data

    cookie = response.message.Data.Cookie
    reader.commit([cookie, ])
    reader.reads_done()


    #  Still can get no more than reads_infly queued read responses before commit response
    for i in range(reads_infly + 1):
        response = reader.next_event().result(timeout=TIMEOUT_SECONDS)
        if response.type != ConsumerMessageType.MSG_COMMIT:
            continue
        else:
            assert response.message.Commit.Cookie == [cookie, ]
    else:
        assert False, "Couldn't get Commit response"


    api.stop()

