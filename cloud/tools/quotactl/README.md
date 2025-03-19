# Cloud Quota Service CLI

## Build

Use `ya make` to build binary.

## Usage

    Usage: quotactl [OPTIONS] COMMAND [COMMAND-OPTIONS]

    OPTIONS:

        --iam-token    Cloud IAM Token (from 'yc iam create-token')
        --endpoint     Service' Private API Endpoint
        --service      Service name (supported: serverless)
        --cloud-id     ID of Cloud to operate on

    COMMAND:

        list-quota     Show all quota values in table format
        set-quota      Set new value for given metric
    
    COMMAND OPTIONS (only for set-quota):

        --name         Name of metric
        --value        Value of metric

## Examples

List quota:

    ./quotactl                                                               \
		--iam-token=$(yc iam create-token)                                   \
		--endpoint=serverless-functions.private-api.ycp.cloud.yandex.net:443 \
		--service=serverless                                                 \
		--cloud-id=$(yc config get cloud-id)                                 \
		list-quota

Set quota:

    ./quotactl                                                               \
		--iam-token=$(yc iam create-token)                                   \
		--endpoint=serverless-functions.private-api.ycp.cloud.yandex.net:443 \
		--service=serverless                                                 \
		--cloud-id=$(yc config get cloud-id)                                 \
		set-quota --name=serverless.workers.count --value=15

