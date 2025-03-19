# goraphite-sender
Graphite sender written in go

# Сборка через ya package

Для сборки нужно https://wiki.yandex-team.ru/devrules/Go/getting-started/
Команда для сборки:
`ya package --debian --key=<YOUR GPG KEY ID> pkg.json`

Сборки и публикации на dist
`ya package --debian --key=<YOUR GPG KEY ID> pkg.json --dupload-no-mail --publish-to=common`
