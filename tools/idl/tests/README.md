```ya make -tt .``` compares files from ```archive.tbz2``` downloaded from the sandbox resource in ```./ya.make``` with the current generation from ```maps/mobile/libs```.

If test fails, inspect diff details in ```./test-results/pytest/testing_out_stuff/stderr``` with your own eyes. If everything is alright, you might want to build MapKit to double-check. Then run ```./update.sh```. It will update  ```archive.tbz2```. Upload your new ```archive.tbz2``` to Sandbox (```ya upload archive.tbz2 --ttl=inf```) and update the resource use in ```./ya.make```.

See [old wiki pages for details](https://wiki.yandex-team.ru/maps/dev/core/mobile/mapkit/autobindings/)
