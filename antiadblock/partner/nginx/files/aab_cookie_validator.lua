

--[[ **********************  Пример использования:  **********************

    ВНИМАНИЕ!!! Модуль aab_aes.lua необходимо положить в папку с остальными сисемными модулями. К примеру, в /usr/local/share/lua/5.1/
    location /hello {
        # Partner key to decrypt and verify cookie
        # WARNING! Do not use plain secrets here! Use only secure mechanisms to pass secrets into conf files.
        set $aab_partner_key {{ lookup('yav', '<secret_version>', '<token_key>') }} ;

        # Cookie name to mark non-adblock surfers
        set $aab_parter_cookie_name 'aab_cookie';

        # TTL of partner cookie, by default - two weeks
        set $aab_parter_cookie_ttl 1209600; # Two weeks (60 х 60 х 24 х 7 х 2)

        # If $aab_debug_output is not null, additional debug info streaming from AntiAdBlock Lua module
        # Information will be streamed with priority saved to $aab_debug_output
        # set $aab_debug_output 0; # Enabling additiona Debug output from AntiAdBlock Lua module

        # If current surfer uses AdBlock apps, $aab_adblock_surfer will be set to 1, else - 0
        set_by_lua_file $aab_adblock_surfer /etc/nginx/sites-available/aab_cookie_validator.lua;

        default_type 'text/html';

        # Using $aab_adblock_surfer in request processing
        content_by_lua_block {
            ngx.say(ngx.var.aab_adblock_surfer)
            if ngx.var.aab_adblock_surfer and tonumber(ngx.var.aab_adblock_surfer) == 1 then
                ngx.say("!!! ADBLOCK user(", ngx.var.aab_adblock_surfer, "), use AntiADB schema!!!")
            else
                ngx.say("regular sirfer (", ngx.var.aab_adblock_surfer, ")")
            end
        }

        # Read Lua files on every request (for debug only)
        # lua_code_cache off;
    }
]]


--[[ **********************  Как проверить работоспособность:  **********************

    Пишем следующий конфиг:

    location /test {
        # Partner key to decrypt and verify cookie
        # WARNING! Do not use real secrets here during test launch - they will appear in nginx logs!
        set $aab_partner_key 'ThisIsPartnerKey';

        # Cookie name to mark non-adblock surfers
        set $aab_parter_cookie_name 'aab_cookie';

        # If $aab_debug_output is not null, additional debug info streaming from AntiAdBlock Lua module
        # Information will be streamed with priority saved to $aab_debug_output
        set $aab_debug_output 0; # Enabling additionaL Debug output from AntiAdBlock Lua module

        # Test launch to check how validator works
        # WARNING! Do not use real secrets here during test launch - they will appear in nginx logs! 
        set $aab_test_launch 1;

        default_type 'text/html';

        # Asking for test page content
        content_by_lua_file /etc/nginx/sites-available/aab_cookie_validator.lua;

        # Read Lua files on every request (for debug only)
        lua_code_cache off;
    }

    После настройки делаем следующий запрос к серверу:
    curl -v -H 'X-Real-Ip: 127.0.0.1' \
        -H 'User-Agent: Curl' \
        -H 'X-Header: 123' \
        -b "some_cook=lslkdfjlskd;aab_cookie=Ya2SMaN6GiuIw2PTC7t4TOPRKixV5OhkqZEtAMR+JzY=;some_other=1" \
        -H 'Accept-Language: Russian-English' \
        'http://localhost/test'

    В результате работы, сервер должен вернуть следующий текст:
        Cookie value: Ya2SMaN6GiuIw2PTC7t4TOPRKixV5OhkqZEtAMR+JzY=
        Partner key: ThisIsPartnerKey
        Decrypted cookie: 0123456789 0123 987654 45678

    Если выскакивает ошибка, или текст не соответствует приведенному, модуль работает не корректно.

]]


-- **************** Implementation section ****************



local aab_aes = require "aab_aes"

--[[
Реализация хеша, используемого в инвертированной схеме
    str - строка, подлежащая хешированию
]]
function hash_string(str)
    local result = 0

    if str~=nil then
        for i = 1, #str do
            result = result + string.byte(str, i) * i
        end
    end

    return result
end

function is_empty_string(str)
    return not str or not string.find(str, '%S') or str == '0'
end

function split( str, delimiter, count)
    local result = { }
    if is_empty_string(str) then
        return result
    end

    local from  = 1
    local idx = 1

    local delim_from, delim_to = string.find( str, delimiter, from  )
    while delim_from and (not count or idx <= count) do
        if delim_from > from
        then
            table.insert(result, idx, string.sub( str, from , delim_from-1 ))
            idx = idx + 1
        end

        from  = delim_to + 1
        delim_from, delim_to = string.find( str, delimiter, from  )
    end

    if from < #str and (not count or idx <= count)
    then
        table.insert(result, idx, string.sub( str, from  ))
    end
    return result
end

--[[
    Собрать информацию о сёрфере и захешировать её для последующего сравнения с содержимым заадблочной куки
    Возвращает:
        nil - В случае, если какой-то из хидеров пуст и информацию полностью собрать не удалось
        table[4] - В случае, если всё ОК
]]
function get_cooked_headers()
    local headers = ngx.req.get_headers()

    local current_time = os.time()

    -- Extracting client IP address
    local client_ip = nil
    if headers["X-Real-Ip"] then 
        client_ip = headers["X-Real-Ip"]
    elseif headers["X-Forwarded-For-Y"] then 
        client_ip = headers["X-Forwarded-For-Y"]
    elseif headers["X-Forwarded-For"] then
        client_ip = split(headers["X-Forwarded-For"], ",", 1)[0]
    end
    if client_ip then
        client_ip = string.sub(client_ip, 1, 50)
    end

    -- Extracting user agent
    local user_agent = headers["User-Agent"]
    if user_agent then
        user_agent = string.sub(user_agent, 1, 300)
    end

    -- Extracting language
    local accept_language = headers["Accept-Language"]
    if accept_language then
        accept_language = string.sub(accept_language, 1, 100)
    end

    -- Computing hashes from current headers values
    local result = { current_time, client_ip, user_agent, accept_language }
    for i = 2, 4 do -- Current time (result[1]) is not string and can not be empty
        if is_empty_string(result[i]) then
            return nil
        end
        result[i] = hash_string(result[i])
    end

    return result
end

function log_message(...)
    if ngx.var.aab_debug_output and not is_empty_string(ngx.var.aab_debug_output) then
        ngx.log(ngx.var.aab_debug_output, "AntiADB-", ngx.var.request_id, ...)
    end
end

--[[
    Пытается определить, является ли сёрфер заадблочным, по инвентированной схеме
    Возвращает 1, если сёрфер заадблочный и 0 в противном случае
]]
function is_adblocked_user()
    local adblock_user_message = "surfer is AdBlocked"

    -- Добываем Антиадблочную куку из заголовока
    local surfer_cookie = ngx.var["cookie_"..ngx.var.aab_parter_cookie_name]
    log_message(" got surfer AAB cookie: ", surfer_cookie)
    if is_empty_string(surfer_cookie) then
        log_message(" ", adblock_user_message, "- AAB cookie is empty")
        return 1
    end

    -- Добываем хидеры для проверки куки
    local hdrs = get_cooked_headers()
    if not hdrs then
        log_message(" ", adblock_user_message, "- one of significant user parameters is empty")
        return 1
    end
    log_message(" got surfer hashes: '", hdrs[1], " ", hdrs[2], " ", hdrs[3], " ", hdrs[4])

    -- Дешифруем присланную куку
    local decrypted_surfer_cookie = aab_aes.decrypt_cookie(ngx.decode_base64(surfer_cookie), string.sub(ngx.var.aab_partner_key, 1, 16))
    if is_empty_string(decrypted_surfer_cookie) then
        log_message(" ", adblock_user_message, "- can't decrypt AAB cookie")
        return 1
    end
    local decrypted_hashes = split(decrypted_surfer_cookie, '%s', 4)
    log_message(" got decrypted hashes: ", decrypted_hashes[1], " ", decrypted_hashes[2], " ", decrypted_hashes[3], " ", decrypted_hashes[4])

    local cookie_ttl = 1209600 -- Two weeks (60 х 60 х 24 х 7 х 2)
    if not is_empty_string(ngx.var.aab_parter_cookie_ttl) then
        cookie_ttl = tonumber(ngx.var.aab_parter_cookie_ttl)
    end

    -- Если TTL куки вышел, считаем сёрфера заадблочным
    if math.abs(tonumber(hdrs[1]) - tonumber(decrypted_hashes[1])) >= tonumber(cookie_ttl) then
        log_message(" ", adblock_user_message, " - AAB cookie TTL is exceeded")
        return 1
    end

    -- Считаем, что кука валидна, если совпадает IP-адрес
    if not is_empty_string(decrypted_hashes[2]) and tonumber(hdrs[2]) == tonumber(decrypted_hashes[2]) then
        log_message(" surfer has no AdBlock software  - AAB cookie TTL is Ok, IP-address is match also")
        return 0
    end

    -- ... ИЛИ агент сёрфера и язык
    if     not is_empty_string(decrypted_hashes[3]) and tonumber(hdrs[3]) == tonumber(decrypted_hashes[3])
       and not is_empty_string(decrypted_hashes[4]) and tonumber(hdrs[4]) == tonumber(decrypted_hashes[4])
    then
        log_message(" surfer has no AdBlock software  - AAB cookie TTL is Ok, agent and language are match also")
        return 0
    end

    log_message(" ", adblock_user_message, " - surfer IP-address or agent\\language do not match")

    return 1
end

if is_empty_string(ngx.var.aab_partner_key) then
    ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: partner key is empty")
    return 1
end

if is_empty_string(ngx.var.aab_parter_cookie_name) then
    ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: partner cookie name is empty")
    return 1
end

if not is_empty_string(ngx.var.aab_test_launch) and tonumber(ngx.var.aab_test_launch)==1 then
    -- Отладочный вывод для настройки модуля на стороне партнёров
    local surfer_cookie = ngx.var["cookie_"..ngx.var.aab_parter_cookie_name]
    ngx.say("Cookie value: ", surfer_cookie)
    local parter_key = string.sub(ngx.var.aab_partner_key, 1, 16)
    -- Uncomment line below to see partner key in logs
    -- ngx.say("Partner key: ", parter_key)
    local decrypted_surfer_cookie = aab_aes.decrypt_cookie(ngx.decode_base64(surfer_cookie), parter_key)
    ngx.say("Decrypted cookie: ", decrypted_surfer_cookie)
else
    -- Боевой режим - возвращаем признак заадблочности
    return is_adblocked_user()
end
