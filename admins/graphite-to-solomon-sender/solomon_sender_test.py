import json
import socket
from collections import namedtuple

from solomon_sender import BinaryFileStorage, MetricDecoder, HealthCheck, \
    CompactingSingleWriterMultipleReaderRingBuffer, AlertStatus, GraphiteSerializer, SolomonJsonSerializer, \
    SimpleSingleWriterMultipleReaderRingBuffer, HealthReporter


def create_empty_buffer(length):
    compactor = MetricDecoder()
    healthcheck = HealthCheck("senders_health_check")
    buffer = CompactingSingleWriterMultipleReaderRingBuffer(length, compactor, healthcheck)
    return buffer


def test_back_up_empty_buffer():
    backup = BinaryFileStorage("test.bin")
    buffer = create_empty_buffer(10)
    backup.pickle(buffer)
    obj = backup.unpickle()
    assert obj


def test_compactor():
    compactor = MetricDecoder()
    metric = b'm1.m2 1 1'
    compacted = compactor.encode(metric)
    decompacted = compactor.decode(compacted)
    assert decompacted == metric


def test_compactor_same_with_gap():
    decoder = MetricDecoder()
    metrics = [b"m1.m1 1 1",
               b"m1.m1 2 2",
               b"m2.m2 3 3",
               b"m1.m1 4 1",
               b"m1.m1 5 2",
               b"m2.m2 6 3"]
    compacted = [decoder.encode(m) for m in metrics]
    decompacted = [decoder.decode(cm) for cm in compacted]
    assert metrics == decompacted


def test_buffer_can_read_what_was_written():
    buffer = create_empty_buffer(10)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    r1_list = buffer.scan("r1", 3)
    assert [b"m1.m1 1 1", b"m1.m1 2 2", b"m2.m2 3 3"] == r1_list


def test_buffer_can_read_what_was_written_even_after_overflow():
    buffer = create_empty_buffer(3)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m1.m1 4 1")
    buffer.write(b"m1.m1 5 2")
    buffer.write(b"m2.m2 6 3")
    r1_list = buffer.scan("r1", 3)
    assert r1_list == [b"m1.m1 4 1", b"m1.m1 5 2", b"m2.m2 6 3"]


def test_buffer_can_read_what_was_written_even_after_overflow2():
    buffer = create_empty_buffer(3)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.shift_read_offset("r1", 1)
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m1.m1 4 1")
    buffer.write(b"m1.m1 5 2")
    r1_list = buffer.scan("r1", 3)
    assert r1_list == [b"m2.m2 3 3", b"m1.m1 4 1", b"m1.m1 5 2"]


def test_buffer_can_read_what_was_written_even_after_overflow1():
    buffer = create_empty_buffer(1)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.shift_read_offset("r1", 1)
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m1.m1 4 1")
    buffer.write(b"m1.m1 5 2")
    r1_list = buffer.scan("r1", 3)
    assert r1_list == [b"m1.m1 5 2"]


def test_buffer_lagging_read():
    buffer = create_empty_buffer(3)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m1.m1 4 1")
    buffer.write(b"m1.m1 5 2")
    assert [b"m2.m2 3 3"] == buffer.scan("r1", 1)
    buffer.write(b"m1.m1 6 2")
    buffer.write(b"m1.m1 7 2")
    assert [b"m1.m1 5 2"] == buffer.scan("r1", 1)


def test_back_up_buffer_with_records():
    buffer = create_empty_buffer(10)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    backup = BinaryFileStorage("test.bin")
    backup.pickle(buffer)
    buffer = backup.unpickle()
    r1_list = buffer.scan("r1", 3)
    assert r1_list == [b"m1.m1 1 1", b"m1.m1 2 2", b"m2.m2 3 3"]


def test_health_check_should_show_warning_if_reader_delay_is_big():
    buffer = create_empty_buffer(4)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m2.m2 3 3")
    buffer.check_health()
    hc = buffer.get_healthcheck()
    assert AlertStatus.WARNING == hc.get_status().status
    buffer.write(b"m2.m2 3 3")


def test_health_check_should_show_CRITICAL_if_WE_LOST_METRICS():
    buffer = create_empty_buffer(2)
    buffer.register_readers(("r1", "r2"))
    buffer.write(b"m1.m1 1 1")
    buffer.write(b"m1.m1 2 2")
    buffer.write(b"m2.m2 3 3")
    buffer.write(b"m2.m2 3 3")
    buffer.check_health()
    hc = buffer.get_healthcheck()
    assert AlertStatus.CRITICAL == hc.get_status().status
    buffer.write(b"m2.m2 3 3")


def test_graphite_serialization_with_prefix():
    serializer = GraphiteSerializer("prefix.")
    m1 = b"m1 1 1"
    m2 = b"m2 2 2"
    assert b"prefix.m1 1 1\nprefix.m2 2 2\n" == serializer.serialize([m1, m2])


def test_graphite_serialization_no_prefix():
    serializer = GraphiteSerializer(None)
    m1 = b"m1 1 1"
    m2 = b"m2 2 2"
    assert b"m1 1 1\nm2 2 2\n" == serializer.serialize([m1, m2])


def test_graphite_serialization_with_float_timestamp():
    serializer = GraphiteSerializer(None)
    m1 = b"m1 1 1.2"
    m2 = b"m2 2 2,5"
    assert b"m1 1 1\nm2 2 2\n" == serializer.serialize([m1, m2])


def test_solomon_serialization():
    common_lables = namedtuple('X', ('host', 'port'))
    common_lables.project = "tv-test"
    common_lables.cluster = "tv-test_test"
    common_lables.service = "tv-test_api"
    serializer = SolomonJsonSerializer(common_lables)
    data = [b"m1.m11.m111 1 1", b"m2.m22.m222 2 2"]
    serialized_data = serializer.serialize(data)
    body = json.dumps(serialized_data, sort_keys=True)
    fqdn = socket.getfqdn()
    assert body == "{\"commonLabels\": {\"cluster\": \"tv-test_test\", \"host\": \"" + fqdn + "\", \"project\": \"tv-test\", \"service\": \"tv-test_api\"}, \"sensors\": [{\"labels\": {\"l0\": \"m1\", \"l1\": \"m11\", \"l2\": \"m111\"}, \"ts\": 1.0, \"value\": \"1\"}, {\"labels\": {\"l0\": \"m2\", \"l1\": \"m22\", \"l2\": \"m222\"}, \"ts\": 2.0, \"value\": \"2\"}]}"


def test_unused_readers_deleted_from_buffer():
    buffer = SimpleSingleWriterMultipleReaderRingBuffer(1, HealthCheck("test"))
    buffer.register_readers(["r1", "r2", "r3"])
    buffer.write("")
    buffer.write("")
    buffer.get_data_loss()

    assert len(buffer.get_last_data_loss().keys()) == 3
    assert len(buffer.get_readers_offsets().keys()) == 3
    buffer.register_readers(["r2", "r4"])
    assert len(buffer.get_last_data_loss()) == 1
    assert len(buffer.get_readers_offsets()) == 2


def test_can_register_readers_only_once():
    buffer = SimpleSingleWriterMultipleReaderRingBuffer(1, HealthCheck("test"))
    buffer.register_readers(["r1", ])
    buffer.write("")
    buffer.write("")
    loss_before = buffer.get_data_loss()
    offset_before = buffer.get_readers_offsets()
    buffer.register_readers(["r1", ])
    loss_after = buffer.get_data_loss()
    offset_after = buffer.get_readers_offsets()
    assert loss_before == loss_after
    assert offset_before == offset_after


def test_new_reader_get_reader_offset_as_writer_offset():
    buffer = SimpleSingleWriterMultipleReaderRingBuffer(1, HealthCheck("test"))
    buffer.write("")
    buffer.write("")
    buffer.register_readers(["r1", ])
    assert buffer.get_readers_offsets().get("r1") == 2


def test_health_reporter_write_status_correctly():
    file = "test_reporter.txt"
    hc1 = HealthCheck("hc1")
    hc2 = HealthCheck("hc2")
    hc1.error("hc1error")
    hc2.error("hc2error")
    hr = HealthReporter(file)
    hr.register_healthcheck(hc1)
    hr.register_healthcheck(hc2)
    hr.report_status()
    with open(file, "r") as f:
        line = f.readline()
        assert "2;CRITICAL:hc1:hc1error;CRITICAL:hc2:hc2error;\n" == line


def test_health_reporter_write_worst_status():
    file = "test_reporter.txt"
    hc1 = HealthCheck("hc1")
    hc2 = HealthCheck("hc2")
    hc1.error("hc1error")
    hc2.warn("hc2warn")
    hr = HealthReporter(file)
    hr.register_healthcheck(hc1)
    hr.register_healthcheck(hc2)
    hr.report_status()
    with open(file, "r") as f:
        line = f.readline()
        assert "2;CRITICAL:hc1:hc1error;WARNING:hc2:hc2warn;\n" == line
