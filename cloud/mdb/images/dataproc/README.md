# Yandex Cloud Data Proc Image
This is a repository for working with data proc bootstrap.
Directory bootstrap contains code and scripts for bootstrap node on instance.
Directory tests contains code for creating dataproc cluster without control-plane with current code and test it with behave.
Directory packer contains code for building images for control-plane.

# How to update
```
arc pull releases/dataproc/dataproc_image_1.4
arc checkout my_1_4_update
<make changes>
arc commit -m "My 1.4 change"
arc pr create --push --to releases/dataproc/dataproc_image_1.4
```

# Assembly workshop notes
Assembly workshop build only one image for both prod and preprod and uses only variables.json
variables-preprod.json and variables-prod.json are used only for manual debugging builds

# Build tasks
[Build base image](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocBaseImage)
[Build public Data Proc image](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImage)
[Move public Data Proc image to preprod public-images folder](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImageMoveToPreprod)
[Move public Data Proc image to prod public-images folder](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImageMove)
