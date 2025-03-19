alias ya='~/Tools/yatool/ya'
echo "Building dutybot"
ya package package.json --docker;
docker push registry.yandex.net/cloud-support-bot:$(docker images | awk '{print $2}' | awk 'NR==2');
rm *.tar.gz
rm dutybot
clear
cat packages.json 
rm packages.json
