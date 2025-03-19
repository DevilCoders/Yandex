Список дисков был получен следующим запросом:

<code>
    USE hahn;

    $parse_disk_id = ($msg) -> {
        RETURN Pire::Capture('Creating volume: \"([a-z0-9]+)\"')($msg);
    };

    $parse_cloud_id = ($msg) -> {
        RETURN Pire::Capture('Creating volume: \"[a-z0-9]*\", \"[a-z0-9]*\", \"[a-z0-9]*\", \"([a-z0-9]+)\"')($msg);
    };

    $parse_media_kind = ($msg) -> {
        RETURN Pire::Capture('Creating volume: \"[a-z0-9]*\", \"[a-z0-9]*\", \"[a-z0-9]*\", \"[a-z0-9]*\", [0-9]+, [0-9]+, ([0-9]+)')($msg);
    };

    SELECT
        disk_id,
        cloud_id,
        media_kind,
        MIN(`timestamp`) as min_timestamp
    FROM (
        SELECT
            $parse_disk_id(`message`) AS disk_id,
            $parse_cloud_id(`message`) AS cloud_id,
            $parse_media_kind(`message`) AS media_kind,
            `message`,
            `timestamp`
        FROM RANGE(
            `home/cloud-nbs/yandexcloud-prod/logs/1d`,
            `2021-03-01`,
            `2021-08-20`)
        WHERE FIND(`message`, "Creating volume: ") is not NULL
        AND FIND(`message`, ", 4096, 4") is not NULL
    )
    GROUP BY disk_id, cloud_id, media_kind
    ;
</code>

Результат положили в disks.txt и обработали следующим скриптом.

<code>
    from collections import defaultdict

    if __name__ == '__main__':
        clouds = defaultdict(int)

        with open('/home/yegorskii/Downloads/disks.txt') as f:
            all_clouds = f.readlines()

            for l in all_clouds:
                fields = l.split('\t')
                clouds[fields[1]] = clouds[fields[1]] + 1

        with open('/home/yegorskii/Downloads/clouds.txt', "w") as of:
            for c, cnt in clouds.items():
                of.write('{},{}\n'.format(c, cnt))
</code>

На выходе получили clouds.txt который подали на вход скрипту cloud/blockstore/tools/analytics/billing-info
 и получили clouds_res.csv
