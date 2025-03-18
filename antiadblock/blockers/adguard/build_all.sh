# Если собираем beta-версию, то надо сгенерить сертификат (там firefox будет в *.xpi формате)
mkdir private && cd private 
mkdir AdguardBrowserExtension && cd AdguardBrowserExtension
openssl genrsa 2048 | openssl pkcs8 -topk8 -nocrypt -out certificate.pem
cd ../..

# Скачивание и сборка блокировщика
yarn install
yarn beta

# меняем нейминг и перетаскиваем в корневую папку для удобства
mv build/chrome.crx adguardchrome.crx
mv build/firefox.xpi adguardfirefox.xpi