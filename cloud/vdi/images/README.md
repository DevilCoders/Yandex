# How to create images

Dont forget to use subnet with exception for PSRP (5986 port)
Dont forget that windows 10 built free by this recepie

## Windows 10

1. Download drivers and extract serial, netkvm + viostor to ```./drivers/[version]/[edition]/*```
1. Download and rename iso for windows 10 into ./iso/windows-10-21h2
1. Build via ```packer build .```
1. Upload ```s3cmd put output/packer-w10ent s3://tempus/windows-10-enterprise.qcow2```
1. Create image

```bash
curl -k \
    -H "Authorization: Bearer `yc iam create-token --profile mkt-mryzh`" \
    -H "Content-type: application/json" \
    -X POST \
    --data '{"folderId":"<folder-id>","name":"windows-10-enterprise-21h2","description":"Windows 10 Enterprise 21H2","os.type":"WINDOWS","minDiskSize":"53687091200","os":{"type":"WINDOWS"},"uri":"<presigned-url>"}' \
    https://compute.api.cloud.yandex.net/compute/v1/images
```

## Image build

```bash
PKR_VAR_zone=<zone-name> \
PKR_VAR_subnet_id=<subnet-id> \
PKR_VAR_token=`yc iam create-token` \
PKR_VAR_folder_id=`yc config list | grep folder-id | cut -d " " -f 2 | tr -d " "` \
packer build .
```
