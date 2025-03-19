let lag = replace_nan({project='kikimr', cluster='<< logbroker.get("cluster") >>', service='pqtabletAggregatedCounters', partition='-', user_counters='PersQueue', sensor='TotalTimeLagMsByLastRead', ConsumerPath='yc.search/search-consumer-<< consumer_name_postfix >>'}, 0);
let maxLag = top(1, 'max', lag);
let maxLagMin = round(max(maxLag)/1000/60);
let topic = get_label(maxLag, 'TopicPath');
alarm_if(maxLagMin >= 120);