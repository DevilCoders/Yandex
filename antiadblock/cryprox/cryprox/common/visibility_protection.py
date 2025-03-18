import re2

from antiadblock.cryprox.cryprox.common.tools.regexp import re_completeurl


INLINE_JS_TEMPLATE = r'''<script {NONCE}type="text/javascript">{INLINE_JS}</script>'''
LOADED_SCRIPT_TEMPLATE = r'''
<script {NONCE}type="text/javascript">
    {SCRIPT_BREAKER_PROTECTION}
    var a=document.createElement("script");
    a.src="{SRC}";
    document.head.appendChild(a);
</script>
'''.replace(' ' * 4, '').replace('\n', '')
XHR_TEMPLATE = r'''
<script {NONCE}type="text/javascript">
    {SCRIPT_BREAKER_PROTECTION}
    var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
    xhr.open("get","{SRC}",!0);
    xhr.onload=function(){{
        var a=document.createElement("style");
        {NONCE_2}
        a.innerHTML=xhr.responseText;
        document.body.appendChild(a);
    }};
    xhr.send();
</script>
'''.replace(' ' * 4, '').replace('\n', '')

PLACEHOLDER = '_classprot_'
FIX_CSS = '._classprot_{visibility: visible!important}'
FIX_JS = r'''
var a=document.createElement("style");
{NONCE_2}
a.innerHTML="._classprot_{{visibility: visible!important}}";
document.body.appendChild(a);
'''.replace('\n', '')
FIX_JS_WITH_XHR = r'''
var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
xhr.open("get","{SRC}",!0);
xhr.onload=function(){{
    var a=document.createElement("style");
    {NONCE_2}
    a.innerHTML=xhr.responseText;
    document.body.appendChild(a);
}};
xhr.send();
'''.replace(' ' * 4, '').replace('\n', '')

HARMFUL_STYLE = '<style {NONCE_2}type="text/css">._classprot_{{visibility:hidden}}</style>'
CLASS_WRAPPER_RE = r'class=\"(?:{class_re})()\"'

XHR_FIX_STYLE_SRC = '//dummy/xhrprotection.css'
SCRIPT_FIX_STYLE_SRC = '//dummy/scriptprotection.js{ARGS}'
NONCE_QUERY_ARG_NAME = "nonce_css"

XHR_FIX_STYLE_SRC_RE = r'dummy/xhrprotection\.css'
SCRIPT_FIX_STYLE_SRC_RE = r'dummy/scriptprotection\.js(?:\?{}=\w+)?'.format(NONCE_QUERY_ARG_NAME)
END_OF_HEAD_RE = r'()</head>'
BEGINNING_OF_HEAD_RE = r'<head.*?>()'
END_OF_BODY_RE = r'()</body>\s*</html>'

# Section for messing up with JavaScript functions Adblock uses
PROPERTY_TO_CHECK_DEFINE_PROPERTY = '__a__'
# breaks Object.currentScript used in AdblockPlus #$#abort-current-inline-script
OBJECT_CURRENT_SCRIPT_BREAKER = r';Object.defineProperty&&Object.getOwnPropertyDescriptor&&Object.defineProperty(document,"currentScript",{get:function(){return null}});'
# uses same methods as Object.currentScript-breaker
SCRIPT_BREAKER_PROTECTION = r'''
document.currentScript="_";
document.currentScript;
if(Object.defineProperty&&Object.getOwnPropertyDescriptor){
    Object.defineProperty(document,"__a__",{value:"__a__",writable:true});
    if("__a__"!=document.__a__)throw Error("Native has been redefined");
}
"string"===typeof document&&
Object.defineProperty&&
Object.getOwnPropertyDescriptor&&
Object.defineProperty(document,"currentScript",{get:function(){return null}});
'''.replace(' ' * 4, '').replace('\n', '')

xhr_fix_style_src_re = re2.compile(re_completeurl(XHR_FIX_STYLE_SRC_RE))
xhr_fix_style_src_re_match = re2.compile(re_completeurl(XHR_FIX_STYLE_SRC_RE, match_only=True))
script_fix_style_src_re = re2.compile(re_completeurl(SCRIPT_FIX_STYLE_SRC_RE))
script_fix_style_src_re_match = re2.compile(re_completeurl(SCRIPT_FIX_STYLE_SRC_RE, match_only=True))
crypt_url_re = re2.compile(re_completeurl([XHR_FIX_STYLE_SRC_RE, SCRIPT_FIX_STYLE_SRC_RE]), re2.IGNORECASE)
crypt_url_re_match = re2.compile(re_completeurl([XHR_FIX_STYLE_SRC_RE, SCRIPT_FIX_STYLE_SRC_RE], match_only=True), re2.IGNORECASE)
crypt_body_re = re2.compile(r'\b{}\b'.format(PLACEHOLDER))
crypt_property_to_check_re = re2.compile(PROPERTY_TO_CHECK_DEFINE_PROPERTY)
