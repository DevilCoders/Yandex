require "stdlib"

function escape(s)
    return (s:gsub('[%-%.%+%[%]%(%)%$%^%%%?%*]','%%%1'):gsub('%z','%%z'))
end

src_root, infile, outfile = arg[1], arg[2], arg[3]
replaces = {}

for i = 4, #arg do
    key, val = string.match(arg[i], "(%w+)=(.*)")
    root_relative_val = string.match(val, escape(src_root) .. "/(.*)")
    if root_relative_val then
        val = root_relative_val
    end
    if key and val then
        replaces[key] = val:gsub("\\","/")
    end
end

infile = io.open(infile, "r")
infile = infile:read("*a")
outfile = io.open(outfile, "w")

result = string.gsub(infile, "/%*@@@([%w_]+)@@@%*/", replaces)

outfile:write(result)
outfile:flush()
outfile:close()
