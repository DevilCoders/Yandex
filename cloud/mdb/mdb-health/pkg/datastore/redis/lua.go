package redis

const (
	// TODO generate and preload lua scripts
	luaScriptLoadHostHealth = `
local fqdn = KEYS[1]
local cid = redis.call('hget', 'fqdn:'..fqdn, 'cid')
if not cid then return {} end
local ctype = redis.call('hget', 'topology:'..cid, 'type')
local result = {cid; ctype}
local key = 'health:'..fqdn
local services = redis.call('hgetall', key)
if services ~= nil then
	local cur_len = #result
	for i=1,#services/2 do
		local service = services[2*i-1]
		local status = services[2*i]
		result[cur_len+2*i-1] = fqdn..':'..service
		result[cur_len+2*i] = status
	end
end
return result`
	luaScriptUpdateTTL = `
redis.call('expire', 'topology:'..KEYS[1], ARGV[1])
local fqdns = redis.call('hget', 'topology:'..KEYS[1], 'fqdns')
if fqdns == nil or fqdns == false then
	return true
end
for fqdn in string.gmatch(fqdns, "[^%s]+") do
	redis.call('expire', 'fqdn:'..fqdn, ARGV[1])
end
return true`
	luaScriptLoadDBHostsFunction = `
local fqdn = KEYS[1]
local hkey = 'fqdn:'..fqdn
local result = redis.call('hmget', hkey, 'cid', 'sid')
if result == nil then
	return {}
end
local cid = result[1]
if cid == nil or not cid then
	return {}
end
local sid = result[2]
local srchosts = 'fqdns'
if sid ~= nil and sid then
	srchosts = 'sid:'..sid
end
local key = 'topology:'..cid
local topinfo = redis.call('hmget', key, srchosts, 'type', 'env', 'sla', 'slashards')
if topinfo == nil then
	return {}
end
local hosts = topinfo[1]
if hosts == nil or not hosts then
	return {}
end
local ctype = topinfo[2]
if ctype == nil then
	return {}
end
result[#result+1] = ctype
result[#result+1] = topinfo[3]
result[#result+1] = topinfo[4]
result[#result+1] = topinfo[5]
result[#result+1] = hosts

local cursor='0'
repeat
	local scan=redis.call('hscan', key, cursor, 'match', 'role:*')
	cursor=scan[1]
	local roleHostsList=scan[2]
	local cur_len = #result
	for i=1,#roleHostsList do
		result[i+cur_len]=roleHostsList[i]
	end
until cursor == '0'

for fqdn in string.gmatch(hosts, "[^%s]+") do
	local services = redis.call('hgetall', 'health:'..fqdn)
	if services ~= nil then
		local cur_len = #result
		for i=1,#services/2 do
			local service = services[2*i-1]
			local status = services[2*i]
			result[cur_len+2*i-1] = fqdn..':'..service
			result[cur_len+2*i] = status
		end
	end
end

return result`
	// language=lua
	luaScriptLoadClusterHostsHealthFunction = `
local function load_cluster_hosts_health(cid)
	local key = 'topology:'..cid
	local result = redis.call('hmget', key, 'fqdns', 'type', 'sla', 'slashards', 'nonaggregatable')
	if result == nil then
		return {}
	end
	local fqdns = result[1]
	if fqdns == nil then
		return {}
	end
	local cursor='0'
	repeat
		local scan=redis.call('hscan', key, cursor, 'match', 'role:*')
		cursor=scan[1]
		local roleHostsList=scan[2]
		local cur_len = #result
		for i=1,#roleHostsList do
			result[i+cur_len]=roleHostsList[i]
		end
	until cursor == '0'

	local cursor='0'
	repeat
		local scan=redis.call('hscan', key, cursor, 'match', 'sid:*')
		cursor=scan[1]
		local sidHostsList=scan[2]
		local cur_len = #result
		for i=1,#sidHostsList do
			result[i+cur_len]=sidHostsList[i]
		end
	until cursor == '0'

	local cursor='0'
	repeat
		local scan=redis.call('hscan', key, cursor, 'match', 'geo:*')
		cursor=scan[1]
		local sidHostsList=scan[2]
		local cur_len = #result
		for i=1,#sidHostsList do
			result[i+cur_len]=sidHostsList[i]
		end
	until cursor == '0'

	cursor='0'
	repeat
		for fqdn in string.gmatch(fqdns, "[^%s]+") do
			local services = redis.call('hgetall', 'health:'..fqdn)
			if services ~= nil then
				local cur_len = #result
				for i=1,#services/2 do
					local service = services[2*i-1]
					local status = services[2*i]
					result[cur_len+2*i-1] = fqdn..':'..service
					result[cur_len+2*i] = status
				end
			end
		end
	until cursor == '0'

	return result
end`
	// language=lua
	luaScriptLoadFewClustersHealth = luaScriptLoadClusterHostsHealthFunction + `
local totalrecs = 0
local type = KEYS[1]
local cursor = KEYS[2]
local limitrecs = tonumber(KEYS[3])
local visible = '1'
local allow_status = { RUNNING = true, MODIFYING = true, ['MODIFY-ERROR'] = true, ['RESTORE-ONLINE-ERROR'] = true}
local result = {cursor}
repeat
	local scan=redis.call('scan', cursor, 'match', 'topology:*')
	cursor=scan[1]
	local cur_len = #result
	for i=1,#scan[2] do
		local meta=redis.call('hmget', scan[2][i], 'type', 'visible', 'status', 'env')
		if meta ~= nil and #meta == 4 and meta[1] == type and meta[2] == '1' and allow_status[meta[3]] then
			local cid = string.sub(scan[2][i], #'topology:*')
			local health = load_cluster_hosts_health(cid)
			totalrecs = totalrecs + 1
			result[cur_len+1] = cid
			result[cur_len+2] = meta[4]
			result[cur_len+3] = health
			result[cur_len+4] = meta[3]
			cur_len = cur_len + 4
		end
	end
until cursor == '0' or totalrecs > limitrecs
result[1] = cursor
return result`
	luaScriptSetInvisible = `
local visible = {}
for _, cid in pairs(KEYS) do
	visible[cid] = true
end

local set_invisible = 0
local cursor = '0'
repeat
	local scan=redis.call('scan', cursor, 'match', 'topology:*')
	cursor=scan[1]
	for i=1,#scan[2] do
		local cid = string.sub(scan[2][i], 10)
		if not visible[cid] then
			local status = redis.call('hget', scan[2][i], 'visible')
			if status == '1' then
				redis.call('hset', scan[2][i], 'visible', '0')
				set_invisible = set_invisible + 1
			end
		end
	end
until cursor == '0'
return set_invisible`
)
