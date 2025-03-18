set -e

# билдим
./tools/make-chromium.sh
./tools/make-firefox.sh
cd dist/build

# меняем версию бандла для аргуса
cd uBlock0.firefox
cat manifest.json | python -c "import sys, json; f = open('manifest.json', 'w'); data = json.load(sys.stdin); data['browser_specific_settings']['gecko']['id'] = 'uBlockOrigin@{}'.format(data['version']); json.dump(data, f); f.close()"
cd ..

# запаковываем
cd uBlock0.chromium && zip -r ../uBlock0.chromium.zip * && cd ..
cd uBlock0.firefox && zip -r ../uBlock0.firefox.xpi * && cd ..
cd ../..
