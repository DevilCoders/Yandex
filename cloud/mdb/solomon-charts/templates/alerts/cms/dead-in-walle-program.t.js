let signal = summary_last(group_lines('sum', {service='yasm', signal='hosts-project-mdb-dead_attt', prj='walle', project='yasm_wallecron', host='/SELF'}));
let alertAggr = max(signal);
alarm_if(alertAggr > 0);
