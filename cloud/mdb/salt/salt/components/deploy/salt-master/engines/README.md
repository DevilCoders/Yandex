## Installation
Copy `prometheus_metrics` code to [engines_dirs](https://docs.saltproject.io/en/latest/topics/engines/index.html#configuration)

Install `prometheus-client`.

Fill master configuration
```yaml
engines:
    - prometheus_metrics:
        # how often collector scans
        # for keys and connected minions
        update_interval: 15
        # port on which metrics exposed
        port: 6060
```

## Metrics
```
# HELP salt_master_keys Number of salt-master minions key
# TYPE salt_master_keys gauge
salt_master_keys{state="denied"} 0.0
salt_master_keys{state="accepted"} 93.0
salt_master_keys{state="rejected"} 0.0
salt_master_keys{state="pending"} 0.0
# HELP salt_master_event Count of salt-master events
# TYPE salt_master_event counter
salt_master_event{tag="run_new"} 3160.0
salt_master_event{tag="minion_start"} 60.0
salt_master_event{tag="job_new"} 10225.0
salt_master_event{tag="key_delete"} 1326.0
salt_master_event{tag="key_accept"} 50.0
salt_master_event{tag="auth_pend"} 1341.0
salt_master_event{tag="minion_ping"} 713.0
salt_master_event{tag="run_ret"} 3160.0
salt_master_event{tag="auth_accept"} 174.0
salt_master_event{tag="minion_refresh"} 240.0
salt_master_event{tag="other"} 82.0
salt_master_event{tag="job_ret"} 9510.0
# HELP salt_master_connected_minions Number of salt-master minions
# TYPE salt_master_connected_minions gauge
salt_master_connected_minions 92.0
# HELP python_info Python platform information
# TYPE python_info gauge
python_info{implementation="CPython",major="2",minor="7",patchlevel="17",version="2.7.17"} 1.0
# HELP salt_master_state_sls Count of state.sls calls
# TYPE salt_master_state_sls counter
salt_master_state_sls{retcode="0",state="components.greenplum.restart_greenplum_cluster",success="true"} 1.0
salt_master_state_sls{retcode="0",state="components.dbaas-operations.service",success="true"} 4.0
# HELP salt_info Salt information
# TYPE salt_info gauge
salt_info{major="3000",minor="9",name="Neon",version="3000.9"} 1.0
# HELP salt_master_func Count of function calls
# TYPE salt_master_func counter
salt_master_func{func="runner.config.get",retcode="unknown",success="true"} 3159.0
salt_master_func{func="runner.auth.mk_token",retcode="unknown",success="true"} 1.0
salt_master_func{func="state.highstate",retcode="0",success="true"} 44.0
salt_master_func{func="saltutil.sync_all",retcode="0",success="true"} 52.0
salt_master_func{func="dbaas.ping_minion",retcode="1",success="false"} 34.0
salt_master_func{func="saltutil.find_job",retcode="0",success="true"} 408.0
salt_master_func{func="test.ping",retcode="0",success="true"} 111.0
salt_master_func{func="mdb_clickhouse.wait_for_zookeeper",retcode="0",success="true"} 2.0
salt_master_func{func="dbaas.ping_minion",retcode="0",success="true"} 8660.0
salt_master_func{func="state.sls",retcode="0",success="true"} 5.0
salt_master_func{func="mdb_clickhouse.resetup_required",retcode="0",success="true"} 6.0
salt_master_func{func="saltutil.is_running",retcode="0",success="true"} 111.0
salt_master_func{func="state.highstate",retcode="2",success="false"} 1.0
salt_master_func{func="state.highstate",retcode="2",success="true"} 21.0
salt_master_func{func="state.apply",retcode="0",success="true"} 3.0
salt_master_func{func="state.running",retcode="0",success="true"} 52.0
```

## Why test with Python3

Cause on Big Sur salt imports fails.
https://github.com/saltstack/salt/issues/57787
