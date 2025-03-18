This library provides two context managers helpful when working with SVN working copy (using console client subprocess calls or using client libraries).

These are **svn_ssh** and **ssh_multiplex**:

```(python)
from library.python import svn_ssh as lpsvn


with lpsvn.svn_ssh(login='Alice', key=get_key_from_secret_storage()):
    # call would be executed on behalf of Alice
    call_svn_status(url)


with lpsvn.ssh_multiplex(repo_url):
    for path in many_paths:
        # call speeds up using ssh multiplexing
        call_svn_update(path)
```

Both methods above work by setting up OS environment variable SVN_SSH properly. Context managers can be safely mixed and/or nested.

If you want manually manage lifetime of multiplexing channel you can use class `Tunnels`:
```(python)
from library.python import svn_ssh as lpsvn


def comb_through_repo():
    # do many calls to svn server
    ...


try:
    lpsvn.Tunnels.ensure_tunnel_url(main_url)
    comb_through_repo()
except SomeSvnError:
    lpsvn.Tunnels.close_tunnel_url(main_url)
    lpsvn.Tunnels.ensure_tunnel_url(replica_url)
    
    call_svn_switch(from=main_url, to=replica_url)
    comb_through_repo()
    lpsvn.Tunnels.close_tunnel_url(replica_url)

```

