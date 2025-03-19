local function split(s, sep)
    sep = sep or ','
    local values = {}
    local pattern = string.format("([^%s]+)", sep)
    for val in string.gmatch(s, pattern) do
        table.insert(values, val)
    end
    return values
end

function add_to_upstream_histogram(name, value)
    for i, v in ipairs(split(value)) do
        val = tonumber(v)
        add_to_histogram(name, val)
    end
end
