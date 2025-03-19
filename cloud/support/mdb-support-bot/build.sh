alias ya='~/Tools/yatool/ya'
echo "Building mdb-support-bot"
ya package package.json --docker;
docker push registry.yandex.net/mdb-support-bot:$(docker images | awk '{print $2}' | awk 'NR==2');
rm *.tar.gz
rm mdb-support-bot
clear
cat packages.json 
rm packages.json
