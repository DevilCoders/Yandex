import time
import json

import yatest_lib.tools


class TraceReportGenerator(object):

    def __init__(self, out_file_path):
        self.File = open(out_file_path, 'w')

    def on_start_test_class(self, class_name):
        self.trace('test-started', {'class': class_name.decode('utf-8')})

    def on_finish_test_class(self, class_name):
        self.trace('test-finished', {'class': class_name.decode('utf-8')})

    def on_start_test_case(self, class_name, test_name, run_dir):
        message = {
            'class': yatest_lib.tools.to_utf8(class_name),
            'subtest': yatest_lib.tools.to_utf8(test_name),
            'logs': {
                'run_dir': run_dir,
            }
        }
        self.trace('subtest-started', message)

    def on_finish_test_case(self, class_name, test_name, status, message, duration, run_dir):
        result = None
        message = {
            'class': yatest_lib.tools.to_utf8(class_name),
            'subtest': yatest_lib.tools.to_utf8(test_name),
            'status': status,
            'comment': message,
            'time': duration,
            'result': result,
            'metrics': {},
            'is_diff_test': False,
            'logs': {
                'run_dir': run_dir,
            }
        }
        self.trace('subtest-finished', message)

    def on_error(self, error, status="fail"):
        self.trace('chunk_event', {"errors": [(status, error)]})

    def trace(self, name, value):
        event = {
            'timestamp': time.time(),
            'value': value,
            'name': name
        }
        self.File.write(json.dumps(event, ensure_ascii=False) + '\n')
        self.File.flush()
