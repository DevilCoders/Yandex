# -*- coding: utf-8 -*-

import json
import sys
import traceback
import yaml
from threading import main_thread, Thread
from flask import Flask

from cloud.mdb.kafka_agent.internal.topic_sync import TopicSync


def main():
    with open('kafka-agent.yaml') as file:
        config = yaml.load(file, yaml.SafeLoader)
    topic_sync = TopicSync(config)

    app = Flask(__name__)

    @app.route("/status")
    def status():
        total_status = {'topic_sync': topic_sync.status()}
        return json.dumps(total_status)

    @app.route("/stacktrace")
    def stacktrace():
        frame = sys._current_frames()[main_thread().ident]
        return f'{main_thread()}\n' + ''.join(traceback.format_stack(frame))

    def run_http_server():
        status_port = config.get('status_port', 5000)
        app.run(host='localhost', port=status_port)

    thread = Thread(target=run_http_server)
    thread.setDaemon(True)
    thread.start()

    topic_sync.run()
