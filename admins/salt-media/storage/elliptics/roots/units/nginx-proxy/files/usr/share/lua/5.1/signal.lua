-- This module is yandex internal.
-- It was inspired by https://github.com/openresty/lua-resty-signal,
-- thanks to @xiaocang github user.
--
-- The main difference is that it does not depend on lua-resty-core module, does
-- not need compilation of C extension, but can't automatically resolve OS-specific
-- signal names.
--
-- USAGE:
--   signal = require "signal"
--
--   -- kill process with SIGKILL
--   signal.kill(<pid>, "KILL")
--
--   -- check process exists, do noting more
--   signal.kill(<pid>, "NONE")

local _M = {
    version = 0.1
}

local ffi = require "ffi"
local C = ffi.C

if not pcall(function () return C.kill end) then
    ffi.cdef("int kill(int32_t pid, int sig);")
end


if not pcall(function () return C.strerror end) then
    ffi.cdef("char *strerror(int errnum);")
end

-- Below is just the ID numbers for each POSIX signal copied from /usr/include/linux/signal.h.
_M.signals = {
    NONE        = 0,

    HUP         = 1,
    INT         = 2,
    QUIT        = 3,
    ILL         = 4,
    TRAP        = 5,
    ABRT        = 6,
    IOT         = 6,
    BUS         = 7,
    FPE         = 8,
    KILL        = 9,
    USR1        = 10,
    SEGV        = 11,
    USR2        = 12,
    PIPE        = 13,
    ALRM        = 14,
    TERM        = 15,
    STKFLT      = 16,
    CHLD        = 17,
    CONT        = 18,
    STOP        = 19,
    TSTP        = 20,
    TTIN        = 21,
    TTOU        = 22,
    URG         = 23,
    XCPU        = 24,
    XFSZ        = 25,
    VTALRM      = 26,
    PROF        = 27,
    WINCH       = 28,
    IO          = 29,
    POLL        = 29,
    PWR         = 30,
    SYS         = 31,
    UNUSED      = 31
}

--- Convert symbolic signal name into signal number for kill command
--- numeric signal name is returned 'as is'
--- @param name string|number - signal name number to convert
--- @return number
function _M.signum(name)
    if type(name) == "number" then
        return name
    end

    local num = _M.signals[name]
    if not num then
        return -1, "unknown signal name '" .. name .. "'"
    end

    return num
end

--- Kill process with regular system call 'kill'
--- @param pid number - ID of process to kill
--- @param sig string - signal name
--- @return boolean, string - <signal_sent>, <error>
function _M.kill(pid, sig)
    if not pid or type(pid) ~= "number" or pid == 0 then
        return nil, "pid number can't be empty"
    end

    local sig_num, err = _M.signum(sig)
    if err then
        return nil, err
    end

    local rc = tonumber(C.kill(pid, sig_num))
    if rc == 0 then
        return true, nil
    end

    err = ffi.string(C.strerror(ffi.errno()))
    return nil, err
end

return _M
