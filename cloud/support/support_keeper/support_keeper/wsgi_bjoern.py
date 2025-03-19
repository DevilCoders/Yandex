import bjoern
import os, signal


from django.core.wsgi import get_wsgi_application

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'support_keeper.settings')
app = get_wsgi_application()

NUM_WORKERS = 8
worker_pids = []
wsgiport = 8080
print(f'Starting bjoern on port {wsgiport}')
bjoern.listen(app, '0.0.0.0', wsgiport)
for _ in range(NUM_WORKERS):
    pid = os.fork()
    if pid > 0:
        # in master
        worker_pids.append(pid)
    elif pid == 0:
        # in worker
        try:
            bjoern.run()
        except KeyboardInterrupt:
            pass
        exit()
try:
   # Wait for the first worker to exit. They should never exit!
   # Once first is dead, kill the others and exit with error code.
    pid, xx = os.wait()
    worker_pids.remove(pid)
finally:
    for pid in worker_pids:
        os.kill(pid, signal.SIGINT)
        exit(1)