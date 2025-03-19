function load_initializing_files(directory)
	local i, popen = 0, io.popen
	local modname = ""
	local _modret = nil
	local pfile = popen('find "'..directory..'"/ -maxdepth 1 -type f -name "*.lua"')
	for filename in pfile:lines() do
		i = i + 1
		modname = string.gsub(filename, ".*/(init_[^/]*).lua$", "%1")
	        ngx.log(ngx.ERR, "try lo load init module: ", modname)
		_modret = require(modname)
	end
	pfile:close()
end

load_initializing_files("/etc/nginx/lua.init.d")
