local stub = {}

local function report_call(fn_name, args)
    ngx.log(ngx.INFO, "[YARL plugin stub]: " .. fn_name .. "('" .. table.concat(args, "', '") .. "')")
end

function stub.limit_by_unique_name(quota, count)
    report_call("limit_by_unique_name", { quota, count })
end
function stub.check_by_unique_name(quota)
    report_call("check_by_unique_name", { quota })
end
function stub.increment_by_unique_name(quota, count)
    report_call("increment_by_unique_name", { quota, count })
end

return stub
