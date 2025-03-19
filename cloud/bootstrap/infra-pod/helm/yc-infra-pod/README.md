# Infra Pod helm chart

## Services

- fluentd+push-client
- unified-agent
- solomon-agent
- juggler-agent

All containers are started in host network namespace and support custom images and tags through imageRepository and
imageTag options.

## Usage

1. Write config for your service (see this doc and examples/ dir for examples) and save it as `service-config.yaml`
2. Install as usual:
```yaml
helm install service-daemonset . -f service-config.yaml
```
New daemonsets and configmaps will be added to cluster:
```
$ kubectl get ds
NAME                                     DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR   AGE
service-daemonset                          1         1         1       1            1           <none>          5d3h
service-daemonset-yc-infra-pod-daemonset   1         1         0       1            0           <none>          4d3h
$ kubectl get cm
NAME                                     DATA   AGE
service-daemonset-yc-infra-pod-configmap   4      4d3h
```

Release name (`service-daemonset`) is used as service pod name by default, but can be redefined with
`servicePodName` parameter

## fluentd+push-client

[Example](examples/push-client.yaml)

Sends logs from stdout/stderr to logbroker using push-client. Supports structured logs via json.
In case of unstructured logs will output simple json:
```json
{"stream":"stdout","logtag":"F","log":"serving on 8888"}
```

Specify `enabled: true` and one of authentication method: tvm, oauth or iam. Add necessary secrets (see **Specifying secrets**)

## unified-agent

[Example](examples/unified-agent.yaml)

Adds unified agent with socket in ```/var/run/unified_agent/{{ Namespace }}-{{ servicePodName }}``` host path.
Uses syslog protocol by default. Native unified-agent procotol can be enabled by specifying the `inputPlugin` parameter.

Specify `enabled: true` and add `statusPort`, output plugin and its configuration. Status port should not overlap
with any port already used on host machine.
Add necessary secrets (see **Specifying secrets**)

## solomon-agent

[Example](examples/solomon-agent.yaml)

Adds solomon agent with yc-solomon-agent-plugins installed.

Specify `enabled: true` and ports:
```yaml
  httpServer:
    bindPort:
  managementServer:
    bindPort:
```
Specify services in format:
```
service_key: <proto.text solomon config>
```
Add necessary secrets (if any) (see **Specifying secrets**)

## juggler-agent

[Example](examples/juggler-agent.yaml)

Adds juggler agent, which executes check from specified bundles or directories.
Checks can be added via custom images or bundles.

Specify `enabled: true` and `instancePort`. By default, only `/etc/monrun` is scanned for checks. Additional dirs
can be specified with `checkDiscoverDirs`. Bundles can be supplied with `checkBundles`

## Local testing on examples
Requirements:
- [Helm >3](https://helm.sh/)
```
helm template test-daemonset . -f examples/push-client.yaml
helm template test-daemonset . -f examples/unified-agent.yaml
helm template test-daemonset . -f examples/solomon-agent.yaml
helm template test-daemonset . -f examples/juggler-client.yaml
```

## Specifying secrets
Secrets are mounted as volumes from /usr/share/yc-secrets host path and specified in form:
```yaml
  secrets:
    {{ id }}:
      name: {{ name }}
      version: {{ version }}
```
name is assumed to be equal to filename

## TODO
- [ ] resources
- [ ] jaeger-agent
- [ ] monitoring
