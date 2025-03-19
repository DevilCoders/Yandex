increment_metric("request_count", 1)
increment_metric("response_" .. ngx.status .. "_count", 1)
increment_metric("response_" .. math.floor(ngx.status / 100) .. "xx_count", 1)
