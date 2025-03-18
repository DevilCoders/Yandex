# sandboxctl 
This project try to implement basic CLI for https://wiki.yandex-team.ru/sandbox
## Get binaries
Sandboxctl is available as ya tool sandboxctl 
```
alias sandboxctl='ya tool sandboxctl'
```
(testenv build job) [https://a.yandex-team.ru/arc_vcs/testenv/jobs/tools/sandboxctl.yaml]
## Build project from scratch
```
# Nothing fancy, just use ya make
ya make  .

# Or build sandboxctl in sandbox via sandboxctl
sandboxctl -W  ya-make tools/sandboxctl/bin/sandboxctl
```
