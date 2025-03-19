# node-icecream
The icecream nodejs frontend.

## Installation
    make install; make watch

Локальный запуск на macOS

1. brew install nvm
. /usr/local/opt/nvm/nvm.sh
nvm install 10.0
nvm use 10.0

2. нужно поставить модули
npm install yarn
yarn

3. чтобы проверить, что всё завелось, нужно захачить то, что скачалось :)
rm -R ./node_modules/@types/react-addons-transition-group/node_modules/@types/react
rm -R ./node_modules/@types/react-addons-css-transition-group/node_modules/@types/react

4. после этого оно должно стартовать без ошибок
yarn start

