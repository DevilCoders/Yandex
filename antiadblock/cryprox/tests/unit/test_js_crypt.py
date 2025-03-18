# coding=utf-8
from urllib import unquote

import re2
import pytest
import cchardet as chardet
from bs4 import BeautifulSoup
from hamcrest import assert_that, equal_to, is_not

from antiadblock.libs.decrypt_url.lib import decrypt_xor
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_inline_js, crypt_js_body, JS_INLINE_TYPES, replace_resource_with_xhr
from antiadblock.cryprox.cryprox.config.system import REPLACE_RESOURCE_WITH_XHR_RE
from antiadblock.cryprox.cryprox.common.tools.regexp import re_merge


@pytest.mark.parametrize('unicode_script,encoding,charset', [
    (u"console.log('some test logging here;')", 'ASCII', 'ASCII'),  # простейший однострочный скрипт
    (u"""<!--
new Image().src = "//counter.yadro.ru/hit?r"+
escape(document.referrer)+((typeof(screen)=="undefined")?"":
";s"+screen.width+"*"+screen.height+"*"+(screen.colorDepth?
screen.colorDepth:screen.pixelDepth))+";u"+escape(document.URL)+
";"+Math.random();//-->""", 'UTF-8', None),  # многострочник
    (u"console.log('тут могла быть ваша реклама;')", 'UTF-8', None),  # простейший однострочный скрипт с юникодом
    (u"console.log('тут могла быть ваша реклама;')", 'UTF-8', 'UTF-8'),
    (u"console.log('тут могла быть ваша реклама;')", 'windows-1251', 'windows-1251'),
    (u"console.log('тут могла быть ваша реклама;')", 'windows-1251', None),  # страница использует неюникодную кодировку, а в заголовке Content-Type это не указано
])
def test_crypt_js_body(unicode_script, encoding, charset):

    seed = 'my2007'
    encoded_script = unicode_script.encode(encoding)
    crypted_script = crypt_js_body(encoded_script, seed, charset=charset)
    # Просто смотрим, что мы действительно пошифровали и текст отличается
    assert_that(crypted_script, is_not(equal_to(encoded_script)))

    # С помощью этой регулярки мы найдем зашифрованный оригинальный скрипт в выводе функции
    crypted_part_template = re2.compile(r"""eval\(decodeURIComponent\(FRZA\(atob\(`([\s\S]+?)`\),`{}`\)\)\)""".format(seed))
    # Получаем шифрованный скрипт, расшифровываем и убеждаемся, что этот тот же самый оригинальный скрипт
    crypted_script_part = crypted_part_template.findall(crypted_script)[0]
    decrypted_script_part = unquote(decrypt_xor(crypted_script_part, seed))

    assert chardet.detect(decrypted_script_part)['encoding'] in ('UTF-8', 'ASCII')  # https://st.yandex-team.ru/ANTIADB-1219
    assert_that(decrypted_script_part, equal_to(unicode_script.encode('UTF-8')))


def test_crypt_inline_js():

    body_original = """
<html>
    <head>
        <meta http-equiv="Content-Type" content="text/html;charset=utf-8">
        <script type="text/javascript">window.o_math = {sign:'532853521',ext_uid:'59db2b567e22d021786485'};</script>
        <link rel="stylesheet" type="text/css" href="//yastatic.net/static/styles.css?v=99" />
        <script src="//yastatic.net/static/glob.js?v=100" type="text/javascript"></script>
        <script src="glob.js?v=100" type="text/javascript">console.log(Math.random());</script>
        <script type="text/javascript">
</script>
        <script type="text/javascript"> </script>
        <script type="text/javascript" src="glob.js?v=100">console.log(Math.random());</script>
    </head>
    <body class='' >
        <noscript>
            <div class="gwd-ad info">Внимание! В Вашем браузере отключена поддержка JavaScript!
            </div>
        </noscript>
        <div class="header">
            <div itemprop="description">Residencia Erasmus расположенный в барселонском районе Грасиа - это то ли отель,
                <p><img class=bigimg src="http://avatars.mds.yandex.net/get-auto/35835321.jpeg" width="600" height="800" alt="Отель Residencia Erasmus Gracia (Испания, Барселона) фото"></p>
            </div>
        </div>
        <script type="text/javascript"><!--
        new Image().src = "//counter.yadro.ru/hit?r"+
        escape(document.referrer)+((typeof(screen)=="undefined")?"":
        ";s"+screen.width+"*"+screen.height+"*"+(screen.colorDepth?
        screen.colorDepth:screen.pixelDepth))+";u"+escape(document.URL)+
        ";"+Math.random();//--></script>

        <link rel='stylesheet' type='text/css' href='//yastatic.net/static/jquery-ui-1.10.4.min.css' />

        <script type="detect">
            !function(e,t,n){function o(){var e=Number(new Date),n=new Date(e+36e5*c.time).toUTCString();t.cookie=c.cookie+"=1; expires="+n+"; path=/"}if(n.src){var c={};for(var r in n)
            n.hasOwnProperty(r)&&(c[r]=n[r]);c.cookie=c.cookie||"bltsr",c.time=c.time||3,c.context=c.context||{},c.context=c.context||function(){},function(e){function n()
            {t.removeEventListener("DOMContentLoaded",n),e()}"complete"===t.readyState||"loading"!==t.readyState&&!t.documentElement.doScroll?e():
            t.addEventListener&&t.addEventListener("DOMContentLoaded",n)}(function()
            {var r={blocked:!0,blocker:"UNKNOWN"},a=new(e.XDomainRequest||e.XMLHttpRequest);a.open("get",n.src,!0),a.onload=function(){try{new Function(a.responseText).call(c.context),
            c.context.init(e,t,c)}catch(e){o(),c.callback(r)}},a.onerror=function(){Number(new Date)-i<1e3&&(o(),c.callback(r))};var i=Number(new Date);a.send()})}}(window,document,{
                src: "https://static-mon.yandex.net/static/main.js?pid=test",
                cookie: "somecookie",
                callback: aab
            });

            function aab(result) {
                if (result.blocked === true) {
                // Есть блокировщик.
                } else {
                // Нет блокировщика.
                }
            };
        </script>
        <script>console.log('some test logging here;')</script>
        <script type="application/json">{"wut": "not
         good"}</script>
        <script type="text/json">{"huh": "dah"}</script>
        <script>try {window.Ya && (window.adfox_150785862505331072 = window.Ya.adfoxCode.create({ownerId: 257765,containerId: 'adfox_150785862505331072',params: {pp: 'g',ps: 'cmmu',p2: 'fskm' }}));}
        catch (e) {}</script><div class="promo-header"><div class="format-1200-90" id="adriver_header"><div id="adfox_150841007796918553"></div><script type="text/javascript">try {window.Ya && ё
        (window.adfox_150841007796918553 = window.Ya.adfoxCode.create({ownerId: 257765,containerId: 'adfox_150841007796918553',params: {'pp': 'g','ps': 'cmmu','p2': 'frul' }}));}
        catch (e) {}</script></div></div><noindex><div id="adriver_banner_fullscreen"></div><script type="text/javascript">try {requireOnDomReady(['rennab-protection'],
        function (rennabProtection) {rennabProtection(function () {try {window.Ya && (window.adriver_banner_fullscreen = window.Ya.adfoxCode.create(
        {ownerId: 257765,containerId: 'adriver_banner_fullscreen',params: {pp: 'g',ps: 'cmmu',p2: 'fscj' }}));} catch (e) {}})})}catch (e) {}
        </script></noindex><div id="adriver_banner_side"></div><script type="text/javascript">try {window.Ya && (window.adriver_banner_side = window.Ya.adfoxCode.create(
        {ownerId: 257765,containerId: 'adriver_banner_side',params: {pp: 'i',ps: 'cmmu',p2: 'fscj' }}));}catch (e) {}</script>
<div id="adfox_151759595329928186"></div><script>try {window.Ya && (window.adfox_151759595329928186 = window.Ya.adfoxCode.create({ownerId: 257765,containerId: 'adfox_151759595329928186',params:
{pp: 'lji',ps: 'cmmu',p2: 'frfe' }}));} catch (e) {}</script>
    </body>
</html>
"""

    seed = 'my2007'
    crypted_body = crypt_inline_js(body_original, seed)

    original_soup = BeautifulSoup(body_original, "lxml")
    crypted_soup = BeautifulSoup(crypted_body, "lxml")

    original_scripts_bodies = [script.string for script in original_soup.findAll('script') if script.string is not None]
    crypted_scripts_bodies = [script.string for script in crypted_soup.findAll('script') if script.string is not None and
                              script.string != '\n' and script.string != ' ' and
                              (script.attrs.get('type', '').find('text/javascript') >= 0 or script.attrs.get('type') is None)]
    original_scripts = [script for script in original_soup.findAll('script')]
    not_crypted_scripts = [script for script in crypted_soup.findAll('script') if script.string not in crypted_scripts_bodies]

    # Проверяем, что мы не сожрали часть скриптов регулярками
    assert len(original_soup.findAll('script')) == len(crypted_soup.findAll('script'))

    # Захардкодил кол-во скриптов, которые, как я ожидаю, должны быть зашифрованы в тестовом html. Так надежнее.
    assert len(crypted_scripts_bodies) == 10

    for crypted_script in crypted_scripts_bodies:
        assert crypted_script not in original_scripts_bodies
    # Проверяем, что скрипты, не попавшие в crypted_scripts не шифрованы (вдруг мы ошиблись в фильтре при формировании crypted_scripts)
    for not_crypted_script in not_crypted_scripts:
        assert not_crypted_script in original_scripts


@pytest.mark.parametrize('script_type', ['application/javascript',
                                         'application/ecmascript',
                                         'text/ecmascript',
                                         'text/javascript',
                                         'application/x-javascript',
                                         'application/json',
                                         'text/json',
                                         'application/java',
                                         'text/javascriptus',
                                         'javascript'])
def test_crypt_inline_js_types(script_type):
    """
    Simple test to check that desired types of script crypted only
    """
    script = '<script type="{}">console.log("are u kidding me?")</script>'.format(script_type)

    crypted_script = crypt_inline_js(script, 'my2007')

    if script_type in JS_INLINE_TYPES:
        assert crypted_script.find('are u kidding me') < 0
    else:
        assert crypted_script == script


@pytest.mark.parametrize('sync', [True, False])
def test_replace_resource_with_xhr(sync):
    body_original = r'''
<html>
    <head>
        <link rel="stylesheet" href="//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1">
        <link rel="stylesheet" href="//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1" />
        <link rel="stylesheet" href="//yandex.ru/vBWQGTk6rxkZqP5laLcgPsydoqg.css">
    </head>
    <body>
        <style src="yandex.ru/static.css"></style>
        <script type="text/javascript" async src="yandex.ru/static.js"></script>
        <script id="someId" src="yandex.net/static.js"></script>
        <script async src="yandex.net/static.js" type="application/javascript"></script>
        <script async
            src="yandex.net/static.js"
            type="application/javascript">
        </script>
        <script src="yandex.ru/static.js" defer></script>
        <script type="text/javascript" src="yandex.by/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
    body_replaced = r'''
<html>
    <head>
        <script type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1",!0);
            xhr.onload=function(){var a=document.createElement("style");
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1",!0);
            xhr.onload=function(){var a=document.createElement("style");
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <link rel="stylesheet" href="//yandex.ru/vBWQGTk6rxkZqP5laLcgPsydoqg.css">
    </head>
    <body>
        <style src="yandex.ru/static.css"></style>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.ru/static.js",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript" id="someId">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="application/javascript";
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="application/javascript";
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript" defer>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.ru/static.js",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();
        </script>
        <script type="text/javascript" src="yandex.by/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
    body_replaced_sync = r'''
<html>
    <head>
        <script type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1",!1);
            xhr.send();
            document.write('<style>'+xhr.responseText+'</'+'style>');
        })();
        </script>
        <script type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//yandex.ru/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1",!1);
            xhr.send();
            document.write('<style>'+xhr.responseText+'</'+'style>');
        })();
        </script>
        <link rel="stylesheet" href="//yandex.ru/vBWQGTk6rxkZqP5laLcgPsydoqg.css">
    </head>
    <body>
        <style src="yandex.ru/static.css"></style>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.ru/static.js",!1);
            xhr.send();
            document.write('<script>'+xhr.responseText+'</'+'script>');
        })();
        </script>
        <script type="text/javascript" id="someId">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!1);
            xhr.send();
            document.write('<script>'+xhr.responseText+'</'+'script>');
        })();
        </script>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!1);
            xhr.send();
            document.write('<script>'+xhr.responseText+'</'+'script>');
        })();
        </script>
        <script type="text/javascript" async>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.net/static.js",!1);
            xhr.send();
            document.write('<script>'+xhr.responseText+'</'+'script>');
        })();
        </script>
        <script type="text/javascript" defer>
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","yandex.ru/static.js",!1);
            xhr.send();
            document.write('<script>'+xhr.responseText+'</'+'script>');
        })();
        </script>
        <script type="text/javascript" src="yandex.by/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
    src_re = re_merge([r'yandex\.ru/static\.js', r'yandex\.net/static\.js', r'//yandex\.ru/\w*?\.css\?check-xhr-working=1'])
    regex = re2.compile(re_merge(REPLACE_RESOURCE_WITH_XHR_RE).format(SRC_RE=src_re), re2.IGNORECASE)
    result = replace_resource_with_xhr(body_original, regex, "", "", sync=sync)

    assert result == (body_replaced_sync if sync else body_replaced)
