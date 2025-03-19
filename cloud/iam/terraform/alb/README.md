1. **Install terraform version 0.13.6**

   Либо скачать нужную версию с Yandex Cloud зеркала https://hashicorp-releases.website.yandexcloud.net/terraform/0.13.6/ (подробности в https://clubs.at.yandex-team.ru/ycp/4790), распаковать и в командах ниже использовать полученный файл.

   Либо использовать [Terraform APT Repository](https://www.terraform.io/docs/cli/install/apt.html)
   ```
   # install terraform=0.13.6
   # https://www.terraform.io/docs/cli/install/apt.html#repository-configuration
   sudo apt-get install terraform=0.13.6
   ```

   Установить `ycp-provider`
   ```
   # install ycp-provider
   curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
   ```

   Детали есть тут <https://wiki.yandex-team.ru/cloud/devel/selfhost/terraform/#m-kakdobavitnovyeresursy> и тут <https://wiki.yandex-team.ru/cloud/devel/terraform-ycp/#install>.

2. **Init terraform**

   ```
   cd <env>
   terraform init -backend-config="secret_key=$(ya vault get version sec-01ewn2b4vf3vmj18c65w1b8wh6 -o AccessSecretKey)"
   ```

3. **Apply changes**

   Для прокатки изменений в окружении `<ENV>` использовать `YCP_PROFILE=ENV` и запускать следующие команды из папки с названием окружения, например для препрода:
   ```
   cd preprod
   YCP_PROFILE=preprod terraform plan
   YCP_PROFILE=preprod terraform apply
   ```

   В конфиге `ycp` должен быть профиль с таким-же названием, как название окружения.
