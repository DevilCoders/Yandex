#!/bin/bash
version=${version:-7.10.0}

echo Donwloading elasticsearch plugins for version $version \\n
folder=plugins-$version
mkdir -p $folder
for plugin in analysis-icu analysis-kuromoji analysis-nori analysis-phonetic analysis-smartcn analysis-stempel analysis-ukrainian discovery-azure-classic discovery-ec2 discovery-gce ingest-attachment mapper-annotated-text mapper-murmur3 mapper-size repository-azure repository-gcs repository-hdfs repository-s3 store-smb transport-nio
do
    echo Downloading plugin $plugin

    url=https://artifacts.elastic.co/downloads/elasticsearch-plugins/$plugin/$plugin-$version.zip
    url_platform=https://artifacts.elastic.co/downloads/elasticsearch-plugins/$plugin/$plugin-linix-x86_64-$version.zip
    # echo $url_platform

    echo Check exist $url_platform
    code=`curl -sIo /dev/null -w '%{http_code}' $url_platform`
    if [ $code -eq 200 ]; then
        url = $url_platform
    fi

    echo Download file $url
    curl -so $folder/$plugin.zip $url
    echo
done
