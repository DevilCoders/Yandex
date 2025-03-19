## Image build

```bash
PKR_VAR_zone=ru-central1-a \
PKR_VAR_subnet_id=e9bn1eq2b8ecio27dlvp \
PKR_VAR_token=`yc iam create-token` \
PKR_VAR_folder_id=`yc config list | grep folder-id | cut -d " " -f 2 | tr -d " "` \
packer build .
```
