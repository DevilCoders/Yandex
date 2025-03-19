local JSON = require "cjson.safe"

local S3Files = {}

--- Find the length of a file.
---
--- @param path string - path to file
--- @return number, string - <size of a file>, <error message>
function S3Files.size(path)
    local fh = io.open(path, "rb")
    if not fh then
        return 0, "failed to open file '" .. path .. "'"
    end

    local len = fh:seek("end")
    if not len then
        return 0, "failed to get size of file '" .. path .. "'"
    end

    fh:close()
    return len, nil
end

--- @return boolean - true when file exists and readable
function S3Files.exists(path)
    local file = io.open(path, "rb")
    if file then file:close() end
    return file ~= nil
end

--- Read an entire file.
---
--- @param path string - path to file
--- @return string
function S3Files.readall(path)
    local fh = io.open(path, "rb")
    if not fh then
        return "", "failed to open file '" .. path .. "'"
    end

    local contents = fh:read(_VERSION <= "Lua 5.2" and "*a" or "a")
    if not contents then
        return "", "failed to read file '" .. path .. "'"
    end
    fh:close()

    return contents, nil
end

--- Read JSON data from file.
---
--- @param path string - path to file
--- @return table - parsed JSON data
function S3Files.readall_json(path)
    local data, err = S3Files.readall(path)
    if err then
        return nil, err
    end

    local decoded
    decoded, err = JSON.decode(data)
    if err then
        return nil, "failed to read file '" .. path .. "' as JSON: " .. tostring(err)
    end
    return decoded, nil
end

--- Write a string to a file.
---
--- @param path string - path to file
--- @param contents string - data to write
--- @return string - error message if any
function S3Files.write(path, contents)
    local fh = io.open(path, "wb")
    if not fh then
        return "failed to open file '" .. path .. "'"
    end
    fh:write(contents)
    fh:flush()
    fh:close()

    return nil
end

--- Write data to file as JSON string
---
--- @param path string - path to file
--- @param contents any - data to wite
--- @return string - error message if any
function S3Files.write_json(path, contents)
    local encoded, err = JSON.encode(contents)
    if err then
        return err
    end
    return S3Files.write(path, encoded)
end

return S3Files
