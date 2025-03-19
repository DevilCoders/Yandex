"""
Simple monitorinf class
"""
class Monitor:
    def __init__(self, filename='/tmp/mongo-refill.status'):
        self.status_filename = filename
        self.write("1;In progress")

    def write(self, status):
        with open(self.status_filename, 'w') as status_file:
            status_file.write(status)
