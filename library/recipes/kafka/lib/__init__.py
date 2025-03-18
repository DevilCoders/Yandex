import argparse
import errno
import logging
import os
import tempfile
import socket
from time import sleep
import yatest.common
from library.python.testing.recipe import set_env
from library.python.testing.recipe.ports import get_port_range, release_port_range


def wait(cmd, process):
    res = process.communicate()
    code = process.poll()

    if code != 0:
        raise RuntimeError("cmd failed: %s, code: %s, stdout/stderr: %s / %s", (cmd, code, res[0], res[1]))

    logging.debug(res[1])


class Kafka:
    def __init__(self, install_dir):
        self.bin_dir = os.path.join(install_dir, "bin")
        self.conf_dir = os.path.join(install_dir, "config")
        self.kafka_handle = None
        self.env = os.environ.copy()
        self.env['JAVA_HOME'] = os.path.dirname(os.path.dirname(yatest.common.java_path()))
        self.kafka_port = 0

    def run(self, kafka_port, dir):
        logging.info("Running kafka...")
        conf = os.path.join(self.conf_dir, 'server.properties')
        log_dir = os.path.join(dir, 'kafka-logs')
        cmd = [
            os.path.join(self.bin_dir, 'kafka-server-start.sh'), conf,
            '--override', 'auto.create.topics.enable=true',
            '--override', 'log.dir={}'.format(log_dir),
            '--override', 'log.dirs={}'.format(log_dir),
            '--override', 'listeners=PLAINTEXT://:{}'.format(kafka_port),
            '--override', 'advertised.listeners=PLAINTEXT://:{}'.format(kafka_port),
            '--override', 'zookeeper.connect={}'.format(os.getenv('RECIPE_ZOOKEEPER_CONNSTRING')),
            '--override', 'offsets.topic.num.partitions=1'
        ]
        self.kafka_handle = yatest.common.execute(cmd, env=self.env, wait=False, shell=True)
        self.kafka_port = kafka_port
        logging.info("Kafka running on port {} with pid {}".format(kafka_port, self.get_kafka_pid()))

    def create_topic(self, topic, partitions):
        logging.info("Creating topic {}".format(topic))
        cmd = [
            os.path.join(self.bin_dir, 'kafka-topics.sh'), '--create',
            '--partitions={}'.format(partitions), '--replication-factor=1',
            '--topic={}'.format(topic),
            '--bootstrap-server={}:{}'.format(socket.gethostname(), self.kafka_port),
        ]
        yatest.common.execute(cmd, env=self.env, wait=True)
        logging.info("Topic {} created".format(topic))

    def get_kafka_pid(self):
        return self.kafka_handle.process.pid


def parse_argv(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('-k', '--kafka-port', type=int, help='Port for Kafka')
    parser.add_argument('-c', '--controller-port', type=int, help='Controller Port for Kafka')
    return parser.parse_args(argv)


def start(argv):
    args = parse_argv(argv)
    if args.kafka_port:
        kafka_port = args.kafka_port
    else:
        kafka_port = get_port_range()
    kafka_dir = tempfile.mkdtemp(prefix='kafka_')
    logging.info("Kafka dir:" + kafka_dir)

    kafka = Kafka(os.path.join(yatest.common.work_path(), 'kafka'))
    kafka.run(kafka_port=kafka_port, dir=kafka_dir)
    parttitons = os.getenv('KAFKA_TOPICS_PARTITIONS', 1)
    topics = os.getenv('KAFKA_CREATE_TOPICS')
    for topic in topics.split(',') if topics is not None else []:
        kafka.create_topic(topic, parttitons)

    with open("kafka_recipe.pid", "w") as pid_file:
        pid_file.write(str(kafka.get_kafka_pid()))

    set_env("KAFKA_RECIPE_BROKER_LIST", 'localhost:{}'.format(kafka_port))
    return kafka_port


def is_running(pid):
    try:
        os.kill(pid, 0)
    except OSError as err:
        if err.errno == errno.ESRCH:
            return False
    return True


def stop(argv):
    args = parse_argv(argv)
    if len(yatest.common.get_param("hang_with_kafka", "")):
        while True:
            continue
    logging.info("Stop Kafka")
    with open("kafka_recipe.pid", "r") as f:
        pid = int(f.read())
    os.kill(pid, 15)

    _SHUTDOWN_TIMEOUT = 10
    seconds = _SHUTDOWN_TIMEOUT
    while is_running(pid) and seconds > 0:
        sleep(1)
    seconds -= 1

    if is_running(pid):
        logging.error('Kafka is still running after {} seconds'.format(seconds))
        os.kill(pid, 9)

    if is_running(pid):
        logging.error("Kafka failed to shutdown after kill 9!")

    if not args.kafka_port:
        release_port_range()
