let failed = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', sensor='search_indexation_parse_failed_documents_*_ammm'});
let unparsedCount = integrate(failed);
let is_red = unparsedCount > 0;

alarm_if(is_red);
