# Выкачиваем патченный адблокплюс и говорим что это adblockplusui
git clone git@github.yandex-team.ru:AntiADB/adblockplus.git
mv adblockplus adblockplusui
npm install

# с установкой пакетов тут почти такая же логика как и в адблокплюсе
cd adblockplusui
# сабмодули и дефолтная скачка пакетов
git submodule update --init --recursive
npm install
# вот тут почему-то install не отрабатывает в сабмодулях в таске, поэтому инсталлим в них отдельно
cd adblockpluschrome && npm install
cd adblockpluscore && npm install
cd ../../..

# сборка блокировщика
npx gulp build -t chrome
npx gulp build -t firefox -c development

# меняем нейминг и перетаскиваем в корневую папку для удобства
mv adblockchrome-*.zip adblockchrome.zip
mv adblockfirefox-*.xpi adblockfirefox.xpi 
