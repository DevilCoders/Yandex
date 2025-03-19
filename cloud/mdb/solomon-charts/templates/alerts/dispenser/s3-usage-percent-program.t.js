let usage = {project='dispenser_common_prod', cluster='dispenser_qloud_env', service='dispenser_db_prod', sensor='pgaas: s3/hdd/hdd-quota', attribute='actual'};
let quota = {project='dispenser_common_prod', cluster='dispenser_qloud_env', service='dispenser_db_prod', sensor='pgaas: s3/hdd/hdd-quota', attribute='max'};

let usage_last = last(usage);
let quota_last = last(quota);
let percent = 100 * usage_last / quota_last;

let is_yellow = percent >= 90;
let is_red = percent >= 95;
let trafficColor = is_red ? 'red' : (is_yellow ? 'yellow' : 'green');

let usage_displayed = to_fixed(usage_last / pow(10, 15), 2);
let quota_displayed = to_fixed(quota_last / pow(10, 15), 2);
let percent_displayed = to_fixed(percent, 2);

alarm_if(is_red);
warn_if(is_yellow);
