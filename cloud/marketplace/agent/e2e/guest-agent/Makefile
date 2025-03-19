.PHONY: windows linux

build_target :=
test_target :=
clean_target :=

ifeq ($(OS),Windows_NT)
	build_target = build-windows
	test_target = test-windows
	clean_target = clean-windows
else
	build_target = build-linux
	test_target = test-linux
	clean_target = clean-linux
endif

all:
	@make --no-print-directory build
	@make --no-print-directory test
	@make --no-print-directory clean

build:
	@make --no-print-directory $(build_target)

test:
	@make --no-print-directory $(test_target)

clean:
	@make --no-print-directory $(clean_target)

build-windows:
	pwsh -noprofile -command 'cd ..; go build -o terratest/agent.exe windows/cmd/yandex-guest-agent/yandex-guest-agent.go'

test-windows:
	pwsh -noprofile -command ' \
		$$ENV:E2E_INSTANCE_PWD = -join ("abcdefghkmnrstuvwxyzABCDEFGHKLMNPRSTUVWXYZ23456789".ToCharArray() | Get-Random -Count 16); \
		$$ENV:E2E_TOKEN = & yc iam create-token; \
		$$ENV:E2E_CLOUD_ID = (& yc config list | ConvertFrom-Yaml)["cloud-id"]; \
		$$ENV:E2E_FOLDER_ID = (& yc config list | ConvertFrom-Yaml)["folder-id"]; \
		& go test -timeout 60m'

clean-windows:
	pwsh -noprofile -command ' \
		Remove-Item -ErrorAction:SilentlyContinue -Force -Recurse terraform/.terraform; \
		Remove-Item -ErrorAction:SilentlyContinue -Force -Recurse config.yaml; \
		Remove-Item -ErrorAction:SilentlyContinue -Force terraform/.terraform.lock.hcl; \
		Remove-Item -ErrorAction:SilentlyContinue -Force terraform/terraform.tfstate; \
		Remove-Item -ErrorAction:SilentlyContinue -Force terraform/terraform.tfstate.backup; \
		Remove-Item -ErrorAction:SilentlyContinue -Force agent.exe; \
		Exit 0'

build-linux:
	cd ..; GOOS=windows go build -o terratest/agent.exe windows/cmd/yandex-guest-agent/yandex-guest-agent.go

test-linux:
	E2E_INSTANCE_PWD=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 16 | head -n 1` \
	E2E_TOKEN=`yc iam create-token` \
	E2E_CLOUD_ID=`yc config list | grep cloud-id | cut -d' ' -f2` \
	E2E_FOLDER_ID=`yc config list | grep folder-id | cut -d' ' -f2` \
	go test -timeout 60m

clean-linux:
	rm -rf terraform/.terraform
	rm -rf config.yaml
	rm -f terraform/.terraform.lock.hcl
	rm -f terraform/terraform.tfstate
	rm -f terraform/terraform.tfstate.backup
	rm -f agent.exe
