let failed = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', sensor='search_indexation_parse_failed_documents_<< cloud_service >>_ammm'});
let parsed = group_lines('sum', {project='<< project_id >>', service='yc-search-stat', cluster='<< indexer.get("cluster") >>', sensor='search_indexation_parsed_documents_<< cloud_service >>_ammm'});
let unparsedCount = integrate(failed);
let parsedCount = integrate(parsed);
let is_red = unparsedCount > 0;

alarm_if(is_red);
