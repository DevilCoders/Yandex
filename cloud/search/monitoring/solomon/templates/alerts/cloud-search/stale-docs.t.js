let stale = group_lines('max', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', sensor='stale_docs_in_index_<< cloud_service >>_axxx'});
let staleCount = integrate(stale);
let is_warn = staleCount > 0;

warn_if(is_warn);
