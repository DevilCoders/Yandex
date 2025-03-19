#!/usr/bin/env python3
import subprocess


class SystemdUnitMonitor(object):

    def __init__(self, service):
        self.service = service

    def is_active_running(self):
        service_properties = {}
        cmd = '/bin/systemctl show %s.service' % self.service
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        stdout_list = proc.communicate()[0].decode('utf-8').split('\n')
        for service_property in stdout_list:
            if service_property:
                k, v = service_property.split("=", 1)
                service_properties[k] = v
        return service_properties['ActiveState'] == 'active' and service_properties['SubState'] == 'running'

if __name__ == '__main__':
    SERVICE = "nbs"
    MONITOR = SystemdUnitMonitor(SERVICE)
    if MONITOR.is_active_running():
        print("PASSIVE-CHECK:{service};0;OK, Service {service} is Active and Running".format(service=SERVICE))
    else:
        print("PASSIVE-CHECK:{service};2;Service {service} is Down".format(service=SERVICE))
