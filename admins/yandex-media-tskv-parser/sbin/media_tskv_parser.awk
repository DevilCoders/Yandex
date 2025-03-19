#!/usr/bin/mawk -f
# use mawk because gawk slower than perl

function parse_fields(){
  if (!initialized){
    delete f
    delete f_names_by_index
    delete v_indexes

    if ($1 != "tskv") { return initialized }

    for (i=2;i<=NF;i++) {
      eq_idx=index($i, "=")
      v_indexes[i]=eq_idx+1
      _n=$i
      # f хранит в себе названия полей tskv лога,
      # типа timestamp, status, total_upstream_timings и т.д.
      # предполагается, что везде они одинаковы, но
      # в диске поле request_time называется response_time,
      if (_n ~ /^(request|response)_time=/) _n="request_time"
      # поэтому выше его приходится 'выпрямить' в правильное название
      # в этом substr операция 'выпрямления' сделанная выше не ломает
      # _f_name, потому-что в _n лежит минимальное по длинне из возможных
      # названий, поэтому substr всегда возвращает полное имя
      _f_name=substr(_n, 1, eq_idx-1)
      f[_f_name]=i
      f_names_by_index[i]=_f_name
    }

    t_up="total_upstream_timings"
    t_req="request_timings"
    t_ssl="ssl_handshake_timings"

    initialized=1
  }
  return initialized
}

# эта функция вытаскивает значение поля, по зарание вычесленному индексу
# на этапе инициализации высчитывается индекс первого, после первого '=' символа.
function v(name) {
  num=f[name]
  if (num) {
    return substr($num, v_indexes[num])
  }
}

# основной skip выполняетя тут, эта функция вызывается перед запуском всех
# счетчиков, если указана опция 'opt_byhandle' то тут формируется prefix
# который потом добавляется во все counter-ы за счет чего происходит подсчет
# с разбиением по vhost или request, opt_byhandle включает разбивку по ручкам
# но для этоно нужно указать -r /request/url и эта часть с заменнными / на '.'
# будет дописана в метрику. Я тут еще добавил разбивку по vhost с опцией -y,
# делает она почти то же самое. Она не отрезает все кроме совпадения,
# а берет весь vhost, пример: '-s -t -y -h api.music.yandex.net -x -r /api'
function _next(){
  if (check_noreq)    if (request      ~ check_noreq)    return 1;
  if (check_novhost)  if (vhost        ~ check_novhost)  return 1;
  if (check_request)  if (request     !~ check_request){
    return 1;
  } else {
    if (opt_byhandle){
      n=match(request, check_request)
      prefix = "handles/"substr(request, n, RLENGTH)"/"
      save_prefix = prefix
    }
  }
  # check_vhost должен обязательно идти после check_request, потому-что
  # внутри проверки vhost используется переменная установленная в проверке
  # check_request переменная 'save_prefix'
  if (check_vhost)    if (vhost !~ check_vhost) {
    return 1;
  } else {
    if (opt_byhost) {
      prefix = vhost"/"save_prefix
    }
  }

  if (group_byfield) {
    value=v(group_byfield)
    if (groupfield_capture) {
        n = match(value, groupfield_capture)
        if (RLENGTH > 0) value = substr(value, n, RLENGTH)
    }
    prefix = value"/"save_prefix
  }
  return 0
}

# просто считает все статус коды и запоминает их под общими именами типа '2xx'
# так же считает total_rps - который значит кол-во запросов а не rps, такое имя
# осталось по историческим причинам
function fix_codes(counter,   lc,cn){
  if (opt_statusNullAsZero){
    counter["status", "0xx"]=0
    counter["status", "1xx"]=0
    counter["status", "2xx"]=0
    counter["status", "3xx"]=0
    counter["status", "4xx"]=0
    counter["status", "5xx"]=0
    counter["status", "total_rps"]=0
  }
  for (cn in counter) {
    _status=substr(cn, index(cn, SUBSEP)+1)
    if (_status ~ /(^|\/)[0-9][0-9][0-9]$/){ # 200 or some/url/200
      lc=length(_status)
      counter["status", substr(_status,1,lc-2)"xx"]+=counter[cn]
      counter["status", substr(_status,1,lc-3)"total_rps"]+=counter[cn]
    }
  }
}

# считает тайминги, на текущий момент эта функция работает в 2х режимах
# в первом режиме (не эффективном и устаревшем) она строит несколько больших
# массивов в памяти. В этих массивах встречается куча повторений,
# как внутри одного массива, так и между массивами.
# В режиме packed функция эффективно упаковывает тайминги, используя времена
# как ключи в мапе, а кол-во одинаковых времен как значения.
# TODO: который возожно не реализуем
# На подумать, как массивы разных таймингов соеденить в один,
# и при этому все еще понимать где чьё и как все это вывести в stdout
function timings_count_inner(name, val, packed, upstream){
  if (upstream) if (v("upstream_response_time") == "-") return
  if (val == "-") return
  # tlens - timings lengths содержит знание того, какие тайминги стоит выводить
  # пользователю, другими словами, какие тайминги указал пользователь.
  # Это значение нужно для знания того, что пользователь хочет видеть и для
  # расчета индекса под новый тайминг (приятный бонус).
  #
  # если в одной строке лога таймингов несколько, то складываем их все
  # это не корректно для upstream_response_time :-(
  # но если думать об upstream как о единой сущности, то вполне сойдет
  n=split(val, _vals, /[ ,:-]+/)
  if (n > 1) for (val = 0;n-- > 0;val += _vals[n+1]);

  # считаем разницу между request_time и суммарным upstream_time,
  # бывает очень полезно в диагностике сетевых проблем
  # считаем, если передан тип diff
  if (opt_diff_timings and name ~ /req_up_diff_timings/)
    val = v("request_time") - val
  # !!!ВАЖНО!!!
  # в разных режимах тайминги эффективнее хранить по разному,
  # это очень сильно влияет на скорость их вывода, см. коммент в print_timings
  if (packed) {
    ++tlens[name]
    timings[name, val]++
  } else {
    timings[name, ++tlens[name]]=val
  }
}
function timings_count() {
  if (opt_up_timings)
    timings_count_inner(prefix""t_up, v("upstream_response_time"), opt_pack_timings)
  if (opt_req_timings)
    timings_count_inner(prefix""t_req, v("request_time"), opt_pack_timings, opt_upstream)
  if (opt_ssl_timings)
    timings_count_inner(prefix""t_ssl, v("ssl_handshake_time"), opt_pack_timings, opt_upstream)
  if (opt_diff_timings)
    timings_count_inner(prefix"req_up_diff_timings", v("upstream_response_time"), opt_pack_timings, opt_upstream)
}

# тупо выводит в stdout весь массив таймигов собранных 'timings_count'
function print_timings(name, as_percentiles, packed){
  # можно конечно отказаться от 'match_by' и пробежаться по массиву используя
  # форму обращения к массиву 2ой размерности типа arr[idx1, idx2]
  # но в этом случае для вывода суммарных значений придется использовать
  # отдельный код и честно говоря смысла в этом мало, потому-что способ
  # описанный ниже работает так же быстро (и даже на несколько наносекунд быстрее)
  match_by = name""SUBSEP
  # формируем правильное имя, в vhost заменяем все '.' на '_',
  # а потом в request заменяем все '/' на '.', при этом '/' может придти не
  # только из request но и через opt_byhost. vhost в префиксе отделяется от
  # request (даже пустого) через добавление '/'
  gsub("\.", "_", name); gsub("/+", ".", name)
  if (as_percentiles)
    print_timings_as_percentile(name, match_by, packed)
  else {
    if (packed){
      # на стороне комбайна Multimetrics понимает, что тайминги упакованы по
      # символу '@' в начале имени таймингов.
      printf "@"name;
    } else {
      printf name;
    }
    for(i in timings){
      if (index(i, match_by) != 0) {
        if (packed){
          # если считаем тайминги в упакованном виде, то выводим их так
          # <значение тайминга>@<кол-во>
          printf " "substr(i, index(i, SUBSEP)+1)"@"timings[i]
        }else{
          # в режиме не упакованного хранения таймингов, эффективнее всего
          # использовать память по полной, потому-что цикл по кол-ву таймингов
          # со сборкой результирующей строки их повторов одинаковых таймингов
          # намного затратнее, чем их изначальное хранение в памяти
          printf " "timings[i]
        }
      }
    }
    print "";
  }
}

# вместо вывода всего массива таймингов, эта функция считает их перцентили
# Отказаться от вызова внешней комманды sort и эффективно считать перцентили на awk
# не получается даже для 'упакованных' таймингов, потому-что на awk пробежаться
# по всему массиву и ПОМУВАТЬ элементы ДОРОЖЕ, чем вызвать внешнюю sort :-(
function print_timings_as_percentile(name, match_by, packed,      data, tcount){
  if (!p_idx) p_idx="100,99,98,97,96,95,94,93,90,75,50"

  # исползуем внешнюю утилиту sort, потому-что это единственный приемлемый
  # по скорости вариант сортировки массива таймингов
  "mktemp /dev/shm/tmp-XXXXXX"|getline tempfile
  command = "sort -nk1,1 -t @ > " tempfile

  for(i in timings) if (index(i, match_by) != 0) {
    # 'tlens' тут не подходит для замены 'tcount', потому-что этот
    # массив не в курсе про размеры суммарных таймингов (опция --byhandle),
    if (packed){
      tcount += timings[i]
      _val = substr(i, index(i, SUBSEP)+1)"@"timings[i]
    } else {
      tcount++
      _val = timings[i]
    }
    print _val | command
  }
  close(command)
  i=0
  while((getline data[++i] < tempfile) > 0);
  close(tempfile)
  system("rm -f " tempfile)

  # вся магия подсчета перцентилей по отсортированному массиву
  # таймингов приведена в строках ниже, а все, что выше - это сортировка
  n=split(p_idx, p, ",")
  for (_pi=1; _pi<=n; _pi++) {
    # расчитываем по фромуле индекс нужного нам тайминга
    _idx = int(tcount*(0.01*p[_pi]))
    if (packed){
      # здесь все немного сложнее, нужно последовательно перебирать имеющиеся
      # уникальные значения таймингов и одновременно считать текущий индекс
      # сравнивать его с искомым индексом, и если текущий тайминг
      # "подходит под описание" запомнить его и остановиться.
      _sumidx = 0
      for (_t=0;_t < tcount;_t++) {
        split(data[_t], _tgt, "@")
        _sumidx += _tgt[2]
        if (_idx <= _sumidx){
          _timing = _tgt[1]
          break
        }
      }
    } else {
      # здесь все просто
      # имеем огромный массив таймингов в памяти и индекс нужного элемента
      _timing = data[_idx]
    }
    printf("%s.%d_prc %0.3f\n", name, p[_pi], _timing)
  }
}

function log_pretty_printer(        sep, f_name, num) {
  if (!fields_initialized){                    # fields_checked should be global
    f_num=split(opt_fields, fields, ",")   # fields and f_num also global
    if (opt_no_fields) {
      f_no_num=split(opt_no_fields, _no_fields, ",")   # f_no_num also global
      for (i=1;i<=f_no_num;i++){
        no_fields[_no_fields[i]] = 1  # field present in no_fields
      }
    }
    fields_initialized=1
  }

  if (opt_raw) {
    sep = "tskv\t"
  } else {
    sep = ""
  }
  if (opt_no_fields) {
    # __i начинается с 2 потому-что на первой позиции идет tskv
    # и в функции инициализациии индексы не меняются.
    __i=2; __NF=NF
  } else {
    __i=1; __NF=f_num
  }

  for (i=__i;i<=__NF;i++){

    if (opt_no_fields){
      f_name=f_names_by_index[i]
      if (no_fields[f_name]) continue
    } else {
      f_name=fields[i]
    }
    if (!f[f_name]) continue # skip missing fields

    if (opt_raw) {
      line=line sep $(f[f_name])
      sep = "\t"
    } else {
      # в дефолтном наборе полей ip идет вторым полем, из-за того что ipv4 и ipv6
      # имеют слишком разные длинны решено делать для этого поля
      # sprintf с выравниванием
      if (f_name == "ip")          line=line sep sprintf("%-24s", v("ip"))
      else if (f_name == "method") line=line sep sprintf("%-4s", v("method"))
      else                         line=line sep v(f_name)
      sep = " "
    }
  }
  print(line); line=""
}


BEGIN {
  FS="\t"

  # Вот этот самый здоровый кусок кода предназначен для парсинга аргументов
  # Положение и начало коммента над каждым else if важно, потому-что оно
  # используется при выводе справки, функция справки парсит этот скрипт и
  # вытаскивает эти комменты.
  for (i = 1; i < ARGC; i++) {
                         # первые [] защищают от вывода этой строки в справке
                         # вторые [] просто экранируют ?
    if      (ARGV[i] ~ /^([-][?]|--help)$/) {
      # если --help зовется через враппер tskv, то хочется подставить в качестве
      # имени программы имя враппера, эту магию делает строка ниже.
      # Враппер в этом случае должен экспортировать переменную MEDIA_TSKV_PARSER_NAME
      if (!ENVIRON["MEDIA_TSKV_PARSER_NAME"]) ENVIRON["MEDIA_TSKV_PARSER_NAME"]=ENVIRON["_"]" -- "

      printf("Использование:\n %s [-о|--опция] [-д|--доп-опция ARG] ... [файл [файл]...]\n"\
        "\nБез параметров эта утилита выводит удобочитаемый формат лога (с дефолтным набором полей)\n"\
        "поданный на stdin, для переопределения выводимых полей смотри -F.\n"\
        "\nВНИМАНИЕ! опции POSIX НЕ совместимы, несколько опций нельзя склеивать вместе"\
        "\nПодробнее тут: https://wiki.yandex-team.ru/media-admin/salt/tmpl/media-tskv\n"\
        "\nПараметры:\n", ENVIRON["MEDIA_TSKV_PARSER_NAME"])
      while((getline l<ENVIRON["_"])>0){
        s=index(l,"\x2F^(-")+3;
        if (desc ~ /^==  /)
          printf("\n%s\n", substr(desc, 3))
        if (s>4){
          have_arg=""
          split(substr(substr(l,s), 1, index(l, ")")-s), cmd, "[|]")
          if (cmd[2]) {
              cmd[1] = cmd[1]","
          } else {
            cmd[2] = cmd[1]
            cmd[1] = ""
          }

          if (desc ~ /^[+]/) {desc=substr(desc, 2);have_arg="ARG"}
          if (desc ~ /^[?]/) {desc=substr(desc, 2);have_arg="[ARG]"}

          printf("    %4s %-26s %s\n", cmd[1], cmd[2]"  "have_arg, desc)
        } else
          desc=substr(l, index(l, "#")+1)
      }
      exit 0
    }

    #==  Фильтры:
    #+- считать только где совпадает vhost
    else if (ARGV[i] ~ /^(-h|--host)$/)     _arg="vhost"
    #+- НЕ считать где совпадает vhost
    else if (ARGV[i] ~ /^(-H|--nohost)$/)     _arg="novhost"
    #+- считать только строки где url совпадает
    else if (ARGV[i] ~ /^(-r|--req)$/)      _arg="req"
    #+- НЕ считать строки где url совпадает
    else if (ARGV[i] ~ /^(-R|--noreq)$/)    _arg="noreq"
    #- считать тайминги только если ходили в upstream
    else if (ARGV[i] ~ /^(-u|--upstream)$/) opt_upstream=1

    #==  Счетчики:
    #?- статус коды + 2xx 3xx и total_rps(!кол-во!! не rps) опционален фильтр regex
    else if (ARGV[i] ~ /^(-s|--status)$/) {opt_status=1;_arg="status_f"}
    #- инициализировать счетчики статус кодов [012345]xx нулем
    else if (ARGV[i] ~ /^(--status-null-as-zero)$/)  {opt_statusNullAsZero=1}
    #- кол-во запросов по ip
    else if (ARGV[i] ~ /^(--ip)$/)                {opt_ip=1}
    #- считать referer-ов
    else if (ARGV[i] ~ /^(--referer)$/)              {opt_referer=1}
    #- запросов по схемам (http,  https)
    else if (ARGV[i] ~ /^(--scheme)$/)               {opt_scheme=1}
    #- количество запросов использующих ssl протокол (по типу протокола)
    else if (ARGV[i] ~ /^(--ssl-protocol)$/)     {opt_ssl_protocol=1}
    #- считать показы капчи ( по ручкам /showcaptcha|/checkcaptcha )
    else if (ARGV[i] ~ /^(-a|--captcha)$/)           {opt_captcha=1}
    #- кол-во запросов в кэш по типу
    else if (ARGV[i] ~ /^(--cache)$/)     {opt_cache_status=1}
    #- кол-во по типу ssl cipher
    else if (ARGV[i] ~ /^(--ssl-cipher)$/)        {opt_ssl_cipher=1}
    #- кол-во по типу antirobot_status
    else if (ARGV[i] ~ /^(--antirobot)$/)    {opt_antirobot_status=1}

    #==  Тайминги:
    #?- тайминги [upstream[,request[,ssl_handshake]]] или up,req,ssl (default: up)
    else if (ARGV[i] ~ /^(-t|--timings)$/) {opt_up_timings=1;_arg="timings"}
    #+- Считать тайминги только по указанным кодам (игнорирует глобальный -s фильтр)
    else if (ARGV[i] ~ /^(--timings-code-filter)$/) {opt_timings_code_filter=1;_arg="timings_f"}
    #- тайминги будут выводиться в упакованном виде (<timing>@<count>)
    else if (ARGV[i] ~ /^(-z|--pack-timings)$/)      opt_pack_timings=1

    #==  Трафик:
    #- трафик в байтах
    else if (ARGV[i] ~ /^(-B|--bytes)$/)             {opt_bytes=1}
    #- трафик в битах
    else if (ARGV[i] ~ /^(-b|--bits)$/)              {opt_bits=1}

    #==  Группировка результата:
    #- добавить в префикс к метрикам кусок совпадения ручки из --req
    else if (ARGV[i] ~ /^(-x|--byhandle)$/) opt_byhandle=1
    #- добавить в префикс к метрикам vhost, обязательно нужен любой --host
    else if (ARGV[i] ~ /^(-y|--byhost)$/) opt_byhost=1
    #+- добавить в префикс к метрикам переданное поле
    else if (ARGV[i] ~ /^(--byfield)$/) _arg="byfield"
    #+- добавить в префикс к метрикам только часть из поля field для группировки
    else if (ARGV[i] ~ /^(--fieldcapture)$/) _arg="fieldcapture"
    #- выводить суммарные значения в режиме --byhandle или --byhost или --byfield
    else if (ARGV[i] ~ /^(-g|--summary)$/) opt_summary=1

    #==  Общие и для консольного использования:
    #- показать справку
    # /^(-?|--help)
    #- парсить формат лога на каждой строке (работает медлеенно!)
    else if (ARGV[i] ~ /^(--slow)$/)     opt_slow_mode=1
    #+- удобочитаемый лог с определенными полями [default: timestamp,ip,status,request_time,method,request]
    else if (ARGV[i] ~ /^(-f|--fields)$/)            {pprint=1;_arg="fields"}
    #+- удобочитаемый лог без определенных полей (через запятую)
    else if (ARGV[i] ~ /^(-F|--no-fields)$/)         {pprint=1;_arg="no_fields"}
    #- выводить неформатированный tskv
    else if (ARGV[i] ~ /^(--raw)$/)                  opt_raw=1
    #?- перцентили (опционально принимает список индексов 100,90,50,30,20,15,10)
    else if (ARGV[i] ~ /^(-p|--percentiles)$/)       {opt_prc=1;_arg="prc_idx"}

    # Все остальное считается не валидным
    else if (ARGV[i] ~ /^--?[a-zA-Z0-9]/) {
      print sprintf("Unrecognized option:  %s", ARGV[i]) > "/dev/stderr"
      # сомнительный момент, может все таки стоит делать exit 1

    } else {
      if (_arg=="vhost")     check_vhost=ARGV[i]
      if (_arg=="novhost")   check_novhost=ARGV[i]
      if (_arg=="req")       check_request=ARGV[i]
      if (_arg=="noreq")     check_noreq=ARGV[i]

      if (_arg=="byfield")      group_byfield=ARGV[i]
      if (_arg=="fieldcapture") groupfield_capture=ARGV[i]

      if (_arg=="fields")    opt_fields=ARGV[i]
      if (_arg=="no_fields") opt_no_fields=ARGV[i]

      # здесь парсятся ключи с опциональным аргументом
      if (_arg=="prc_idx" && ARGV[i] ~ /^[0-9]+(,[0-9]+)*$/) p_idx=ARGV[i]
      # фильтры статус кодов
      if (_arg=="status_f") check_status=ARGV[i]
      if (_arg=="timings_f") check_timings_status=ARGV[i]

      # типы считаемых таймингов
      if (_arg=="timings" && ARGV[i] ~ /^[a-z_,]+$/) {
        # выключаем upstream тайминги, потому-что есть опциональный аргумент,
        # если upstream тайминги перечислены в нем, они включатся опять
        opt_up_timings=0
        types_len=split(ARGV[i], timings_types, ",")
        for (t_idx in timings_types) {
          # включает upstream тайминги если -t получила тип u(pstream)?
          if (timings_types[t_idx] ~ /^u/) { opt_up_timings=1 }
          # включает request тайминги если -t получила тип r(equest)?
          if (timings_types[t_idx] ~ /^r/) { opt_req_timings=1 }
          # включает ssl тайминги если -t получила тип s(sl_handshake)?
          if (timings_types[t_idx] ~ /^s/) { opt_ssl_timings=1 }
          # включает diff тайминги если -t получила тип d(iff)?
          if (timings_types[t_idx] ~ /^d/) { opt_diff_timings=1 }
          # Example: -t up,req,ssl or -t up,r,ssl_handshake or --timings ssl,u
        }
      }
    }
    # аргументы для опции должны идти строго после опции, поэтому тут
    # сбрасываем признак того, что опция требует аргумент
    if (_arg && _arg_set_in_prev_step==_arg) {_arg="";_arg_set_in_prev_step=""}
    if (_arg) _arg_set_in_prev_step=_arg
    delete ARGV[i]
  }

  if (!opt_fields)
    opt_fields="timestamp,ip,status,request_time,method,request"
}{
  while (!parse_fields()) { next }
  # initialized 100% должен быть в 1 на строке после while с parse_fields
  # поэтому тут можно его переопределить и тем самым получить
  # медленный режим на котором каждая строчка будет парситься по новой
  initialized = !opt_slow_mode


  status_code=v("status")
  request=v("request")
  vhost=v("vhost")

  # проверка status кода не должна прерывать формироватние префикса
  if (!opt_timings_code_filter) {
    if (check_status && status_code !~ check_status) { next }
  }
  # Глобальный фильтр _next формирует префиксы для opt_byhandle и opt_byhost,
  # поэтому она всегда должна быть первой в основном цикле, но если
  # фильтр по кодам таймингов не указан, то проверка статус кода до _next
  # может сильно выиграть по производительности
  if (_next()) { next }

  if (opt_timings_code_filter){
    if (status_code ~ check_timings_status) timings_count()

    # эта провека тут призвана прервать дальнейшее выполнение, если оно
    # отфильтруется по глобальному фильтру
    if (check_status && status_code !~ check_status) { next }
  }

  # утилита для удобного чтения логов
  # она вызывается только в 2х случаях, эта функция выполняет реформатирование
  # tksv лога в удобочитаемый набор значений определенных полей из лога
  # 1. отсутствия аргументов, набор полей по умолчанию, см. в код функции
  # 2. c агрументом -f <fields,list> выводятся только значения указанных полей
  if (pprint) {
    log_pretty_printer()
    next
  }

  # если opt_timings_code_filter не указан, значит считаем тайминги
  # по глобальному фильтру
  if (!opt_timings_code_filter) timings_count()

  # все что идет ниже - это непосредственный подсчет всех метрик, который были
  # запрошены через опции парсера

  # prefix нужен для случая, если пользователь позвал '--byhandle'
  # если да, то в prefix будет кусок урла с замененными '/' на '.' и
  # в конце будет точка

  if (opt_captcha) {
    # вытаскиваем из запроса (show|check|status)captcha? и отрезаем captcha?
    # не уверен есть ли там status, но кажется преположение о его существовании
    # ничего не ломает, просто в переменную captcha попадет лишний символ в конце
    # полученный ключ используем в качестве имени метрики
    captcha=substr(request,2,12)
    captcha_idx=index(captcha, "captch")
    if (captcha_idx > 2)  # индекс с 2 просто чтобы не попасть на начало строки
      counter["captcha", prefix"captcha/"substr(captcha,1,captcha_idx-1)]++
  }
  # считаем всякие счетчики, где кол-во можно посчитать тупо инкриментом
  if (opt_status)       counter["status", prefix""status_code]++
  if (opt_cache_status) counter["upstream_cache_status", prefix""v("upstream_cache_status")]++
  if (opt_scheme)       counter["scheme", prefix""v("scheme")]++
  if (opt_ssl_protocol) counter["ssl_protocol", prefix""v("ssl_protocol")]++
  if (opt_ip)           counter["ip", prefix""v("ip")]++
  if (opt_referer)      counter["referer", prefix""v("referer")]++
  if (opt_antirobot_status)    counter["antirobot_status", prefix""v("antirobot_status")]++
  # TODO: BUG: FIXME: нужно избавиться от ключей вида '-', ну или забить.
  if (opt_ssl_cipher)   counter["ssl_cipher", prefix""v("ssl_cipher")]++


  if (opt_bytes || opt_bits) traffic[prefix"total"]+=v("bytes_sent")

}
# дальше вся магия с выводом полученных счетчиков
END {
  if (opt_status)
    fix_codes(counter);

  for (c in counter){
    metric_name=substr(c, index(c, SUBSEP)+1)
    # формируем правильное имя, в vhost заменяем все '.' на '_',
    # а потом в request заменяем все '/' на '.', при этом '/' может придти не
    # только из request но и через opt_byhost. vhost в префиксе отделяется от
    # request (даже пустого) через добавление '/'
    gsub("\.", "_", metric_name)
    # так же тут сохраняется host_name - это то, что было в vhost на момент
    # сбора метрик в counter.
    # Это нужно для суммарного значения кодов по vhost без учета request
    host_name=substr(metric_name, 1, index(metric_name, "/")-1)
    gsub("/+", ".", metric_name)

    print metric_name, counter[c]

    # тут от каждой метрики забирается последний элемент пути, (2xx,301,...)
    # по этому последнему элементу считаются глобальные значения
    len=split(metric_name, path, ".")
    summary_metrics_counts[path[len]]+=counter[c]

    # В идеальном случае в host_name должны попадать только vhost
    # потому-что / может приехать только с opt_byhandle
    if (opt_byhost && host_name) {
      summary_byhost_metrics_counts[host_name"."path[len]]+=counter[c]
    }
  }
  for (n in tlens) {
      print_timings(n, opt_prc, opt_pack_timings)
  }

  for (t in traffic){
    name=t
    gsub("\.", "_", name);
    host_name=substr(name, 1, index(name, "/")-1)
    gsub("/+", ".", name)
    len=split(name, path, ".")
    summary_traffic+=traffic[t]
    if (opt_byhost && host_name) {
      summary_traffic_by_host[host_name"."path[len]]+=traffic[t]
    }
    if (opt_bits)
        printf name"_bits_sent %.0f\n", traffic[t] * 8;
    if (opt_bytes)
        printf name"_bytes_sent %.0f\n", traffic[t];
  }

  # суммарные метрики, считаются на этапе вывода основных метрик
  # если используется режим разбиения по ручкам и есть фильтр по ручкам
  # эта секция обретает смысл.
  if ((opt_byhandle && check_request || group_byfield) && opt_summary){
    if (opt_byhost) {
      # сумарные метрики по хостам нужны только если были включены метрики
      # по ручкам, иначе сумма по хостам уже посчиталась в основном цикле
      # и тут это пересчитывать не нужно.
      for (hs in summary_byhost_metrics_counts)
        print hs, summary_byhost_metrics_counts[hs]

      for (ts in summary_traffic_by_host) {
        if (opt_bits)
            printf ts"_bits_sent %.0f\n", summary_traffic_by_host[ts] * 8;
        if (opt_bytes)
            printf ts"_bytes_sent %.0f\n", summary_traffic_by_host[ts];
      }
    }
    for (c in summary_metrics_counts)
        printf "%s %.0f\n", c, summary_metrics_counts[c];

    if (opt_bits)
        printf "total_bits_sent %.0f\n", summary_traffic * 8;
    if (opt_bytes)
        printf "total_bytes_sent %.0f\n", summary_traffic

    for (n in tlens) {
        gsub("\.", "_", n); gsub("/+", ".", n)
        len=split(n, path, ".")
        # тут от каждого имени тайминга забирается последний элемент пути (тип),
        # путь разбивается по '.', и по этому последнему элементу выводятся
        # все тайминги, получаются глобальные тайминги по типу (up,request,...)
        if (!uniq[path[len]]++)
           print_timings(path[len], opt_prc, opt_pack_timings)
    }
  }
}

# vim: sw=2 et:
