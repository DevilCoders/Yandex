# corba-mongo-grants
Grants generator for mongodb

# source
Moved from https://github.yandex-team.ru/admins/corba-mongo-grants

## Build debian package
see https://wiki.yandex-team.ru/yatool/package

build
`ya package --change-log changelog --debian --key=<YOUR GPG KEY ID> pkg.json`

build and publish
`ya package --change-log changelog --dupload-no-mail --debian --key=<YOUR GPG KEY ID> pkg.json --publish-to=common`
