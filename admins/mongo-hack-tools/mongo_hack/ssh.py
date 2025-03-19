import paramiko


class SSHNonZeroError(RuntimeError):
    pass


class SSH (object):
    def __init__(self, host):
        self.host = host
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.client.connect(host, username='root', timeout=10)

    def __del__(self):
        self.client.close()

    def run(self, command, dump=True, stdin=None):
        if dump or True:
            print '[%s]  { %s }' % (self.host, command)
        channel = self.client.get_transport().open_session()
        channel.set_combine_stderr(True)
        channel.exec_command(command)
        if stdin:
            channel.sendall(stdin)
            channel.shutdown_write()
        ofile = channel.makefile()

        data = []
        line = ofile.readline()
        while line:
            if dump:
                print '[%s]  %s' % (self.host, line.strip())
            data.append(line)
            line = ofile.readline()

        exitcode = channel.recv_exit_status()
        if exitcode:
            raise SSHNonZeroError("Process terminated with code %s" % exitcode)

        return ''.join(data).strip()
