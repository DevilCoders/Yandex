## Deploy IAM services release into datacloud environment

**Prerequisites**: You need [kubectl](#install-kubectl), [helm](#install-helm), [sops](#install-sops)
and [helm-secrets](#install-helm-secrets) installed.

### Install aws-cli
https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html
#### Linux (debian-based)
```
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip
sudo ./aws/install
```
#### macOS
```
curl "https://awscli.amazonaws.com/AWSCLIV2.pkg" -o "AWSCLIV2.pkg"
sudo installer -pkg AWSCLIV2.pkg -target /
```

### Install kubectl
#### Linux (debian-based)
```
# Update the apt package index and install packages needed to use the Kubernetes apt repository:
sudo apt-get update
sudo apt-get install -y apt-transport-https ca-certificates curl

# Download the Google Cloud public signing key:
sudo curl -fsSLo /usr/share/keyrings/kubernetes-archive-keyring.gpg https://packages.cloud.google.com/apt/doc/apt-key.gpg

# Add the Kubernetes apt repository:
echo "deb [signed-by=/usr/share/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list

# Update apt package index with the new repository and install kubectl:
sudo apt-get update
sudo apt-get install -y kubectl
```
Read more: https://kubernetes.io/docs/tasks/tools/install-kubectl-linux/#install-using-native-package-management
#### macOS
```
brew install kubectl
```
or
```
brew install kubernetes-cli
```
Read more: https://kubernetes.io/docs/tasks/tools/install-kubectl-macos/#install-with-homebrew-on-macos

### Install Helm
#### Linux (debian-based)
```
# Update the apt package index and install packages needed to use the Helm apt repository:
curl https://baltocdn.com/helm/signing.asc | sudo apt-key add -
sudo apt-get install apt-transport-https --yes

# Add the Helm apt repository:
echo "deb https://baltocdn.com/helm/stable/debian/ all main" | sudo tee /etc/apt/sources.list.d/helm-stable-debian.list

# Update apt package index with the new repository and install helm:
sudo apt-get update
sudo apt-get install -y helm
```
Read more: https://helm.sh/docs/intro/install/#from-apt-debianubuntu
#### macOS
```
brew install helm
```
Read more: https://helm.sh/docs/intro/install/#from-homebrew-macos

### Install SOPS
#### Linux (debian-based)
```
wget https://github.com/mozilla/sops/releases/download/v3.7.1/sops_3.7.1_amd64.deb
sudo dpkg -i sops_3.7.1_amd64.deb 
```
Read more: https://github.com/mozilla/sops/blob/master/README.rst
#### macOS
```
brew install sops
```
Read more: https://github.com/mozilla/sops/blob/master/README.rst

### Install helm-secrets
```
helm plugin install https://github.com/jkroepke/helm-secrets --version v3.6.0
```
Read more: https://github.com/jkroepke/helm-secrets#using-helm-plugin-manager

## Deploy IAM-Services
On error `Error: found in Chart.yaml, but missing in charts/ directory: datacloud-lib`
just run 
```
helm dependency update
```
at helm-chart repository (for example datacloud/helm/datacloud-deployment).


```
cd datacloud/bootstrap/terraform/deployments/preprod/aws/
```
```
terraform init
```
```
terraform plan
```

## Secrets
We store secrets at Arcadia VCS encrypted. To edit secret use:
```
helm secrets edit <path>/secrets.yaml
```
Read more: https://wiki.yandex-team.ru/datacloud/operation/secrets/

##Certificates
[Certificates README.md](root-ca/README.md)

## Drain Node
Use kubectl drain to remove a node from service.

List k8s-nodes:
```
kubectl get nodes
```
Drain node you want to destroy:
```
kubectl drain <node name>
```
NB! After that, we need to wait about of half an hour until the DNS caches will be flushed. And only then can we delete the nodes.

Read more: https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/#use-kubectl-drain-to-remove-a-node-from-service

**NB!** it mau be more usefull to cordon node and redeploy all pod to evict them without load-balancing problems. And after that node can be drained.
