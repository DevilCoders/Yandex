import argparse


def get_argparser():
    parser = argparse.ArgumentParser(description='Yandex.Cloud Support CLI.', prog='saint', usage='''
    saint --init                                            generate config file
    saint --clear-cache                                     clear cached IAM-token
    saint -bin <6_first_digits>                             show card info by BIN
    saint -c <cloud>                                        show cloud info by cloud_id
    saint -c <cloud> -af                                    show all folders in cloud and cloud info
    saint -c <cloud> -ru                                    resolve all users in cloud and show cloud info
    saint -c <cloud> -ba                                    show billing and cloud info
    saint -c <cloud> [subcmd] --owners                      show cloud info with owners list
    saint -f <folder>                                       show cloud info by folder_id
    saint -d <disk_id>                                      show cloud and disk info by disk_id
    saint -b <billing_id>                                   show billing id and clouds
    saint -b <billing_id> --grants                          show billing id and clouds and active grants
    saint -b <billing_id> --dead-grants                     show billing id and clouds and dead/expired grants
    saint -p <promocode>                                    show promocode info
    saint -s3 {bucket_name | cloud_id}                      show cloud and S3 bucket info / all buckets in cloud
    saint -s3 <bucket_name> -mv <folder_id>                 move bucket to specified folder_id
    saint -i <instance_id>                                  show instance info
    saint -i <instance_id> -m                               show instance metadata
    saint -i <instance_id> -o                               show instance and operations
    saint -i <instance_id> --disks | -ad                    show instance and attached disk
    saint -i <instance_id> --nets  | -ni                    show instance and attached network interfaces
    saint -ip <ip-address>                                  show instance by IP-address
    saint -acc <user_id|user_login>                         show all user permissions and roles on resources
    saint -acc <user_id|user_login> --organization          same, for user belonging to an organization
    saint -ai <cloud_id>                                    show all instances in cloud
    saint -ai <cloud_id> -ss                                show all instances in cloud with subnet_id
    saint -mdb {--id | --fqdn | --shard | --node} <id>      show cluster info by custom id
    saint -mdb {custom_id} <id> -o                          show cluster and operations
    saint -cr <cloud>                                       show all clusters in cloud
    saint -ips <cloud>                                      show all IP-addresses in cloud
    saint -sn <cloud>                                       show all subnets in cloud
    saint [cmd] [subcmd] --sort <field>                     sort table by field name
    saint [cmd] [subcmd] {--json | --yaml}                  show info in json/yaml format
    saint [cmd] [subcmd] --key <key>                        show only specified key
    saint -simg                                             show all standard images for VMs
    saint -img <image_id>                                   show image info'''
                                     )
    # Clouds, users, k8s
    cloud_group = parser.add_argument_group('Cloud, billing and users')
    cloud_group.add_argument('--cloud', '--сдщгв', '-c', '-с', type=str, help='resolve creator login with specified <cloud_id>')
    cloud_group.add_argument('--all-folders', '-af', '-фдд-ащдвукы', '-фа', action='store_true',
                             help='show all folders. --cloud <cloud_id> required')
    cloud_group.add_argument('--billing-account', '-ba', '--ишддштп-фссщгте', '-иф',action='store_true',
                             help='resolve billing info when using --cloud or --login.')
    cloud_group.add_argument('--folder', '-f', '--ащдвук', '-а', type=str, help='show folder info')
    cloud_group.add_argument('--users', '-ru', '--гыукы', '-кг', action='store_true',
                             help='show all users in cloud, --cloud <cloud_id> required')
    cloud_group.add_argument('--owners', '--щцтукы', action='store_true',
                             help='show owners email for cloud resolve (maybe SLOW)')
    cloud_group.add_argument('--get-mk8s-masters', '-gmk8s', '--пуе-ьл8ы-ьфыеукы', '-пьл8ы', type=str,
                             help='return mk8s masters by mk8s cid')
    cloud_group.add_argument('-vmk8s', '--verbose-masters', '-мл8ы', '--мукищыу-ьфыеукы', action='store_true',
                             help= 'resolve zonal cluster masters instances')
    # Billing thingies
    cloud_group.add_argument('--billing', '-b', '--ишддштп', '-и', type=str, help='resolve billing by BA ID.')
    cloud_group.add_argument('--grants', '-g', '--пкфтеы', '-п', action='store_true', help='get grants, -b <billing_id> required')
    cloud_group.add_argument('--dead-grants', '-dg', '--вуфв-пкфтеы', '-вп', action='store_true', help='get dead grants, -b <billing_id> required')
    cloud_group.add_argument('--bin-check', '-bin', '--ишт-срусл', '-ишт', type=str,
                             help='show card info by BIN (6 first digits)')
    # ToDo: payments and promocodes
    cloud_group.add_argument('--payments', '-pa', '--зфньутеы', '-зф', action='store_true', help='get payments history, -b <billing_id> required')
    cloud_group.add_argument('--promocode', '-p', '--зкщьщсщву', '-з', type=str, help='check promocode')

    # Instances
    instance_group = parser.add_argument_group('Instances')
    instance_group.add_argument('--instance', '-i', '--штыефтсу', '-ш', type=str, help='show instance information')
    instance_group.add_argument('--all-instances', '-ai', '--фдд-штыефтсуы', '-фш', metavar='CLOUD', type=str,
                                help='show all instances for cloud id')
    instance_group.add_argument('--ip-address', '-ip', '--шз-фввкуыы', '-шз', type=str, metavar='IP', help='resolve instance by IP-address')
    instance_group.add_argument('--metadata', '-m', '--ьуефвфеф', '-ь', action='store_true',
                                help='show instance metadata, -i <instance_id> required')
    instance_group.add_argument('--disks', '-ad', '--вшылы', '-фв', action='store_true',
                                help='show attached disks. -i <instance> required')
    instance_group.add_argument('--nets', '-ni', '--туеы', '-тш', action='store_true',
                                help='show attached network interfaces. -i <instance> required')
    instance_group.add_argument('--disk', '-d', '--вшыл', '-в', type=str, help='show disk info and resolve to cloud_id')
    instance_group.add_argument('--get-node', '-gn', '--пуе-тщву', '-пт', type=str, help='return compute node')

    # Accounts and Security
    accs_group = parser.add_argument_group('Accounts and security')
    accs_group.add_argument('--account', '-acc', '-фсс', '--фссщгте',  type=str, help='get info and roles of selected account')
    accs_group.add_argument('--organization', '--щкпфтшяфешщт', type=str, help='to look in yandex team use yc.organization-manager.yandex')
    # Managed databases
    mdb_group = parser.add_argument_group('Managed Databases')
    mdb_group.add_argument('--managed-db', '-mdb', '--ьфтфпув-ви', '-ви', action='store_true',
                           help='cluster resolve, required --id / --fqdn / --shard / --node')
    mdb_group.add_argument('--id', '--шв', type=str, help='cluster id for resolve')
    mdb_group.add_argument('--fqdn', '--айвт', type=str, help='fqdn for resolve')
    mdb_group.add_argument('--shard', '--ырфкв', type=str, help='shard id for resolve')
    mdb_group.add_argument('--node', '--тщву', type=str, help='instance id for resolve')
    mdb_group.add_argument('--clusters', '-cr', '--сдгыеукы', '-ск', metavar='CLOUD', type=str, help='show all clusters for <cloud_id>')
    mdb_group.add_argument('--db-config', '-cfg', '--ви-сщташп', '-сап', action='store_true', help='feature. now disabled')

    # Object storage group
    s3_group = parser.add_argument_group('Object storage')
    s3_group.add_argument('--bucket', '-s3', '--игслуе', '-ы3', type=str, metavar='S3', help='resolve S3 bucket by name / cloud')
    s3_group.add_argument('--move-to', '-mv', '--ьщму-ещ', '-ьм', type=str, metavar='folder_id',
                          help='move bucket to folder. use with -s3 arg')
    # Network group
    net_group = parser.add_argument_group('Network')
    net_group.add_argument('--subnets', '-sn',  '--ыгитуеы', '-ыт',type=str,
                           help='set this flag to resolve all networks with their subnets for <cloud_id>')
    # net_group.add_argument('--network', '-nt', type=str, help='resolve network to cloud_id (disabled)')
    net_group.add_argument('--addresses', '-ips', '--фввкуыыуы', '-шзы', type=str, metavar='CLOUD', help='show all external IP-addresses')

    # Access group
    access_group = parser.add_argument_group('Access group')
    access_group.add_argument('--user')
    # Additional group
    addons_group = parser.add_argument_group('Optional arguments')
    addons_group.add_argument('--debug', '-v',  '--вуигп', '-м', action='store_true', help='debug info')
    addons_group.add_argument('--clear-cache', '-cc', '--сдуфк-сфсру', '-сс', action='store_true', help='clean cache for IAM-token')
    addons_group.add_argument('--version', '--мукышщт', action='version', version='saint 1.3.3')
    addons_group.add_argument('--init', '--штше', action='store_true', help='generate config file and download CA cert')
    addons_group.add_argument('--operations', '-o', '--щзукфешщты', '-щ', action='store_true', help='show resource operations')
    addons_group.add_argument('--json', '--оыщт', action='store_true', help='return data in json format')
    addons_group.add_argument('--yaml', '--нфьд', action='store_true', help='return data in yaml format')
    addons_group.add_argument('--key', '--лун', type=str, metavar='JSON_KEY', help='show only specified key')
    addons_group.add_argument('--sort', '--ыщке', type=str, metavar='FIELD', help='sort table by field name')
    addons_group.add_argument('--profile', '--зкщпшду', type=str, metavar='PROFILE',
                              help='use specific profile: prod, preprod, etc.')
    addons_group.add_argument('--pre', '--зку', action='store_true', help='A shortcut for preprod environment')
    addons_group.add_argument('--standard-images', '-simg', '--ыефтвфкв-шьфпуы', '-ышьп', action='store_true', help='get standard images list')
    addons_group.add_argument('--get-image', '-img', '--пуе-шьфпу', '-шьп', type=str, help='get image info')
    return parser
