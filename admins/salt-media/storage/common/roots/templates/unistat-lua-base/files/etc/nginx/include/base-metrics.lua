if ngx.var.prj == nil then
    prj = 'notset'
else
    prj = ngx.var.prj:gsub("[^a-z0-9-]", "-")
end
increment_metric(string.format("prj=%s;tier=%s;request_count_dmmm", prj, ngx.status), 1)
add_to_histogram(string.format("prj=%s;tier=%s;request_timings_hgram", prj, ngx.status), ngx.var.request_time)
if ngx.var.upstream_response_time ~= nil then
  add_to_upstream_histogram(string.format("prj=%s;tier=%s;request_upstream_timings_hgram", prj, ngx.status), ngx.var.upstream_response_time)
end
