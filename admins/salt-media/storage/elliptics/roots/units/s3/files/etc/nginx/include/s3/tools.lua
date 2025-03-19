local tools = {}

local function measure_time (func, location)
    local begin_time = ngx.now()
    local threshold = tonumber(ngx.var.time_threshold)
    func()
    local exec_time = ngx.now() - begin_time
    if exec_time > threshold
    then
        ngx.log(ngx.ERR, string.format("lua code location: %s, exec time: %.3f", location,  exec_time))
    end
end

tools.measure_time = measure_time

return tools
