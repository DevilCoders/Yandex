
TIMEOUT_SECONDS = 5
from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import PQStreamingAPI, ConsumerMessageType
from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import ProducerConfigurator, ConsumerConfigurator

if __name__ == "__main__":
    api = PQStreamingAPI("logbroker.yandex.net", 2135)
    f = api.start()
    f.result(TIMEOUT_SECONDS)

    simple_writer_config = ProducerConfigurator(topic='yacloud-cloudai--yacloud-cloudai-billing', source_id='api_test_writer')
    writer = api.create_producer(simple_writer_config)

    response = writer.start_future.result(timeout=TIMEOUT_SECONDS)
    assert response.HasField('Init')
    current_seq_no = response.Init.MaxSeqNo

    f = writer.write(current_seq_no + 1, "Hello world")
    write_result = f.result(timeout=TIMEOUT_SECONDS)

    f = writer.write(current_seq_no + 2, "Hello world2")
    write_result = f.result(timeout=TIMEOUT_SECONDS)

    api.stop()

