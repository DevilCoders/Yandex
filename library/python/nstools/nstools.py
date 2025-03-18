Fs = -1
Ipc = -1
Network = -1
Mount = -1
Pid = -1
User = -1
Uts = -1

def unshare_ns(flags):
    raise RuntimeError("Namespaces aren't available in your OS")

def move_to_ns(fileobject, mode):
    raise RuntimeError("Namespaces aren't available in your OS")
