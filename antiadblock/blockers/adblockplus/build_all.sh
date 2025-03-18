# сабмодули и дефолтная скачка пакетов
git submodule update --init --recursive
npm install

# вот тут почему-то install не отрабатывает в сабмодулях в таске, поэтому инсталлим в них отдельно
cd adblockpluschrome && npm install
cd adblockpluscore && npm install
cd ..

# сборка блокировщика
npx gulp build -t chrome
npx gulp build -t firefox -c development
cd ..

# меняем нейминг и перетаскиваем в корневую папку для удобства
mv adblockpluschrome/adblockpluschrome-*.zip adblockpluschrome.zip
mv adblockpluschrome/adblockplusfirefox-*.xpi adblockplusfirefox.xpi