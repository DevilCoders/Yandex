all:
#	@make --no-print-directory build
	@make --no-print-directory test-with-terraform-destroy
	@make --no-print-directory clean

#build:
#	[ -f guest-agent-updater.exe ] \
#		GOOS=windows \
#			go build \
#				-o guest-agent-updater.exe \
#				windows/cmd/yandex-guest-agent-updater/yandex-guest-agent-updater.go

test-with-terraform-destroy:
	E2E_TOKEN=`yc iam create-token` \
	E2E_CLOUD_ID=`yc config list | grep cloud-id | cut -d' ' -f2` \
	E2E_FOLDER_ID=`yc config list | grep folder-id | cut -d' ' -f2` \
	GOOS=linux \
	go test -timeout 60m -count=1

test:
	E2E_NO_DESTROY="TRUE" \
	E2E_TOKEN=`yc iam create-token` \
	E2E_CLOUD_ID=`yc config list | grep cloud-id | cut -d' ' -f2` \
	E2E_FOLDER_ID=`yc config list | grep folder-id | cut -d' ' -f2` \
	GOOS=linux \
	go test -timeout 60m -count=1

clean:
	cd terraform &&\
	TF_VAR_token=`yc iam create-token` \
	TF_VAR_cloud_id=`yc config list | grep cloud-id | cut -d' ' -f2` \
	TF_VAR_folder_id=`yc config list | grep folder-id | cut -d' ' -f2` \
	terraform destroy -auto-approve &&\
	cd - &&\
	rm -rf terraform/.terraform &&\
	rm -f terraform/.terraform.lock.hcl &&\
	rm -f terraform/terraform.tfstate &&\
	rm -f terraform/terraform.tfstate.backup
	#rm -f guest-agent-updater.exe
