#!/bin/bash -e


if [[ ! -n $SCALE ]]; then
  if [[ -n $1 ]]; then
      SCALE=$1
  else
      SCALE=100
  fi
fi

if [[ ! -n ${TABLE} ]]; then
  TABLE="hits_${SCALE}m_obfuscated"
fi
DATASET="${TABLE}_v1.tar.xz"

# Note: on older Ubuntu versions, 'axel' does not support IPv6. If you are using IPv6-only servers on very old Ubuntu, just don't install 'axel'.

FASTER_DOWNLOAD=wget
if command -v axel >/dev/null; then
    FASTER_DOWNLOAD=axel
else
    echo "It's recommended to install 'axel' for faster downloads."
fi

if [[ ! -d data ]]; then
      if [[ ! -f $DATASET ]]; then
          if [[ -n $CLOUD_RUN ]]; then
            echo "Please check that you have an access to GitHub. You may not have it if you are running the script inside ipv6-only network like Yandex.Cloud ClickHouse host. You need to either download the dataset manually https://clickhouse-datasets.s3.yandex.net/hits/partitions/$DATASET or remove the exit statement in this script if you are sure you have an access to GitHub"
            exit 1
          fi

          $FASTER_DOWNLOAD "https://clickhouse-datasets.s3.yandex.net/hits/partitions/$DATASET"
      fi

      tar $TAR_PARAMS --strip-components=1 --directory=. -x -v -f $DATASET

      if [[ -n $CLOUD_RUN ]]; then
        DATA_TARGET="/var/lib/clickhouse/store/000/00000000-0000-0000-0000-000000000000"
        METADATA_TARGET="/var/lib/clickhouse/metadata/default/"
        mkdir -p $DATA_TARGET

        # TODO: fix 201307_1_96_4
        sudo cp -r "data/default/${TABLE}/201307_1_96_4" $DATA_TARGET
        sudo cp -r "metadata/default/${TABLE}.sql" $METADATA_TARGET

        sudo chown -R clickhouse:clickhouse $DATA_TARGET
        sudo chown -R clickhouse:clickhouse $METADATA_TARGET

        sudo service clickhouse-server restart
      fi
fi