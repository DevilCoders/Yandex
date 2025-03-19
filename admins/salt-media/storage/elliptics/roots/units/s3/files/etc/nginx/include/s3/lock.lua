-- This script was inspired by https://github.com/openresty/lua-resty-lock
-- Thanks to Yichun Zhang (agentzh)

local shared = ngx.shared

local Lock = {}
local lockMT = { __index = Lock }

--- Create new locker
--- @param dict_name string - the nginx shared dict to use to store lock.
--- @param lock_name string - the key in nginx shared dict to use as lock.
--- @param locker_id string - the locker name or ID to see in lock value. Helps to identify the current lock keeper
--- @return table - locker object bound to shared dict
function Lock.new(_, dict_name, lock_name, locker_id)
    if not lock_name then
        return nil, "lock name can't be empty"
    end

    local dict = shared[dict_name]
    if not dict then
        return nil, "dictionary '" .. dict_name .. "' not found"
    end

    local lockObject = {
        dict = dict,
        id = locker_id,
        lock_name = lock_name,
        locked = false,
    }

    return setmetatable(lockObject, lockMT), nil
end

--- Try to obtain lock.
--- @param self table - lock object from Lock.new()
--- @param expire_time number - lock expiration time in seconds with max precision 0.1 second
--- @return boolean, string
function Lock.try_lock(self, expire_time)
    local dict = self.dict
    local locker_id = self.id

    local ok, err = dict:add(self.lock_name, locker_id, expire_time)
    if ok then
        self.locked = true
        return true
    end

    if err ~= "exists" then
        return false, err
    end

    -- someone already got the lock
    return false, "locked by: " .. tostring(dict:get(self.lock_name))
end

--- Release the lock
--- @param self table - lock object from Lock.new()
function Lock.unlock(self)
    local dict = self.dict
    local locked = self.locked

    if not locked then
        return
    end

    self.locked = false

    local lock_owner = dict:get(self.lock_name)
    if lock_owner == nil then
        -- the lock is already free because of TTL
        return
    end

    if lock_owner ~= self.id then
        -- the lock is owned by someone else, not us
        return
    end

    -- Here we still have small window for race between latest lock check and delete below.
    -- Have no idea how to avoid it. :(
    -- The only way is to make sure lock's TTL is much longer than max execution time of code that
    -- acquires and releases the lock
    dict:delete(self.lock_name)
    return
end

return Lock
