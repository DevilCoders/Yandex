let responseCodes400_498 = group_lines('sum', {project='<< project_id >>', cluster='<< proxy.get("cluster") >>', service='yc-search-stat', sensor='search-400-498_ammm'});
let responseCodes499 = group_lines('sum', {project='<< project_id >>', cluster='<< proxy.get("cluster") >>', service='yc-search-stat', sensor='search-499_ammm'});
let responseCodes5xx = group_lines('sum', {project='<< project_id >>', cluster='<< proxy.get("cluster") >>', service='yc-search-stat', sensor='search-codes-5xx-*'});
let responseCodes400_498_count = integrate(responseCodes400_498);
let responseCodes499_count = integrate(responseCodes499);
let responseCodes5xx_count = integrate(responseCodes5xx);
let is_alarm = (responseCodes400_498_count + responseCodes499_count + responseCodes5xx_count) > 0;

alarm_if(is_alarm);
