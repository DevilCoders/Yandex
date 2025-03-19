# HowToStart

Scratch of how to start, didn't test it so pr's are welcome.

## Compile

Go to root of cmd's code.

```bash
cd $(go env -json | jq '.GOPATH' | tr -d '"')/src/a.yandex-team.ru/cloud/marketplace/mktctl
```

Compile'n'throw.

```bash
ya make -tt && cp cmd/mktctl/mktctl ~/go/bin/
```

Source it

```bash
. ~/.bashrc
```

## Config

Create directory for config.

```bash
mkdir -p $HOME/.config/mktctl
```

Create basic self-explanatory config.

```bash
cat <<EOF > $HOME/.config/mktctl/config.yaml
current: preprod
profiles: 
  prod:
    federation-id: yc.yandex-team.federation
    federation-endpoint: console.cloud.yandex.ru
    marketplace-console-endpoint: mkt.private-api.cloud.yandex.net:443
    marketplace-partners-endpoint: mkt.private-api.cloud.yandex.net:443
    marketplace-private-endpoint: mkt.private-api.cloud.yandex.net:8443
  preprod:
    federation-id: yc.yandex-team.federation
    federation-endpoint: console-preprod.cloud.yandex.ru
    marketplace-console-endpoint: mkt.private-api.cloud-preprod.yandex.net:443
    marketplace-partners-endpoint: mkt.private-api.cloud-preprod.yandex.net:443
    marketplace-private-endpoint: mkt.private-api.cloud-preprod.yandex.net:8443 

EOF
```

## Completion

Generate completions file.

```bash
mktctl completion bash >> $HOME/.config/mktctl/completion.bash.inc
```

Add hook

``` 
cat <<EOF >> $HOME/.bashrc

# The next line enables shell command completion for mktctl.
if [ -f '$HOME/.config/mktctl/completion.bash.inc' ]; then source '$HOME/.config/mktctl/completion.bash.inc'; fi

EOF
```

Source it.

```bash
. ~/.bashrc
```
