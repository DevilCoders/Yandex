# -*- coding: utf8 -*-
import requests

import antiadblock.libs.adb_selenium_lib.browsers.extensions as extensions

COOKIES_OF_THE_DAY_URL = 'https://proxy.sandbox.yandex-team.ru/last/ANTIADBLOCK_ALL_COOKIES'
COOKIES_OF_THE_DAY_LIST = requests.get(COOKIES_OF_THE_DAY_URL).text.splitlines()

SELENIUM_EXECUTOR = "localhost:4444"


# language=JavaScript
DETECT_SCRIPT = """var result_aab = arguments[arguments.length - 1];
!function(){var r,f=["y2fSBgjHy2S=","BhvKy2e=","Bg9HzgLUzW==","vfjvu1rfrcbysfiGqu5tv0vs","y29TlMDL","BgvUz3rO","ic0G","zxzLBNq=","z2v0uMvZCg9UC2vizwfKzxi=","C2v0vgLTzw91Da==","BwvZC2fNzq==","zNjVBunOyxjdB2rL","rNvUy3rPB24=","zxHWAxjLCW==","Aw4UDwe=","DgLTzxn0yw1W","vu5ltK9xtG==","oYbtyw1Lu2L0zt1oB25LoYbZzwn1CMu=","C3rHy2S=","ywrKrxzLBNrmAxn0zw5LCG==","CMvZCg9UC2vuzxH0","ue9tva==","Bg9JyxrPB24=","u1rbuLq=","y29TlNrY","C3bSAxq=","B25SB2fK","y29VA2LL","z2v0q29TChv0zwrtDhLSzq==","C3bIlNj1","mI4XlJa=","AM9PBG==","ChvZAa==","Dg9gAxHLza==","CgfNzwHPzgu=","zwXLBwvUDa==","zxzLBNrFzgv0zwn0x0LoteLorq==","CMvHzhLtDgf0zq==","y3LJywrH","ChjVDg90ExbL","zgv0zwn0x0LoteLorq==","rxjYB3iGD2HPBguGC3vIC2nYAwjPBMCGre9nq29UDgvUDeXVywrLza==","rgf0zq==","yMvMB3jLDw5SB2fK","BgfIzwXZ","weHsiefou1DfuG==","Ahr0Chm6lY9HBI55yw5KzxGUCNuVANn0CMfJzxi=","C2vUzejLywnVBG==","B3bLBG==","yMX0C3i=","y29TlMfT","y29UDgv4Da==","yMXVy2TLza==","weHsihjLCxvLC3qGzxjYB3i=","zxzLBNruExbL","DgfNCW==","Dhj1C3rLza==","zNvUy3rPB24=","werVBwfPBLjLCxvLC3q=","re9nieXpqurfra==","BMfTzq==","C2XPy2u=","oYbLEhbPCMvZpq==","ywrKAxrPB25HBa==","weHsifjfuvvfu1q=","w29IAMvJDcbbCNjHEv0=","sgvHzgvYigXLBMD0AcbKB2vZBID0ig1HDgnO","re9difjfqurz","C3rLChm=","weHsievsuKjbq0S=","we1mshr0CfjLCxvLC3q=","zg9ty3jVBgW=","CMvTB3zLrxzLBNrmAxn0zw5LCG==","tNvTyMvY","su5msu5f","weHsigz1BMn0Aw9UCYb3zxjLihjLyxnZAwDUzwq=","D2L0AenYzwrLBNrPywXZ","BxnRlNj1","ywfIx2rLDgvJDa==","veLnru9vva==","DgLTzw91Da==","Dg9tDhjPBMC=","y2XLyxjuAw1LB3v0","BMf2AwDHDg9Y","vw5RBM93BG==","Dg9vventDhjPBMC=","yMLUza==","yNrVyq==","zgv2AwnL","vgLTzw91Da==","tK9ux0jmt0nlruq=","zg9JDw1LBNrfBgvTzw50","yMvLCMTH","C2v0sxrLBq==","AhjLzG==","Ag9ZDg5HBwu=","zgf0yq==","y28UAwW=","oYbWyxrOps87igrVBwfPBJ0U","y29UDgvUDc1Szw5NAhq=","C2vSzG==","zxzLBNroyw1L","AgfZt3DUuhjVCgvYDhK=","Dg9W","weHsignVBNn0CNvJDg9YigvYCM9Y","C3jJ","DMvYC2LVBG==","zg9JDw1LBNrnB2rL","rw1WDhKGy29UzMLNlNnYyW==","CgLK","sLnptG==","Aw5KzxHpzG==","yMXVy2TLCG==","yNjVD3nLCG==","y2HHCKnVzgvbDa==","DMfSDwu=","DgLTzq==","B25LCNjVCG==","C3rHDhvZvgv4Da==","Aw5PDa==","re9nq29UDgvUDeXVywrLza==","C3rYAw5NAwz5","y29TCgXLDgu=","C2vUza==","y2fSBa==","yKHwA1KYrt0=","Bg9JywXtDg9YywDL","u3rHDhvZoIa=","Aw5MCMfTzq==","u3rYAw5N","C2vYDMLJzq==","zg9TywLU","r0vu"];r=f,function(x){for(;--x;)r.push(r.shift())}(119);var X=function(x,r){var n=f[x=+x];void 0===X.DlEDrf&&(X.NaznFf=function(x){for(var r=function(x){for(var r,n,v=String(x).replace(/=+$/,""),f="",a=0,t=0;n=v.charAt(t++);~n&&(r=a%4?64*r+n:n,a++%4)&&(f+=String.fromCharCode(255&r>>(-2*a&6))))n="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/=".indexOf(n);return f}(x),n=[],v=0,f=r.length;v<f;v++)n+="%"+("00"+(function(a,b){return" !\\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~".indexOf(a[b])+32})(r,v).toString(16)).slice(-2);return decodeURIComponent(n)},X.GwumuN={},X.DlEDrf=!0);var v=X.GwumuN[x];return void 0===v?(n=X.NaznFf(n),X.GwumuN[x]=n):n=v,n};!function(o,y,x){var r,g=X,B={};for(r in x)x[g("0x75")](r)&&(B[r]=x[r]);var D=[g("0x27"),g("0x13"),g("0x41"),g("0x70"),g("0x5c"),g("0x2c"),g("0x1d")],h=o[g("0x49")]||o[g("0x55")],n=g("0x78"),v=g("0x47"),N=g("0x2a"),A=g("0x83"),t=g("0x42"),f=g("0xf"),M=g("0x35"),a=g("0x6b"),j=g("0x10"),u=g("0x7"),q=g("0x3d");B[N]=B[N]||g("0x40");try{({})[g("0x60")][g("0x6")](B[N])!==g("0x50")&&(B[N]=[B[N]])}catch(x){}B[A]=B[A]||336,B[t]=B[t]||{};var w=B[f];if(B[f]=V,B[n]){var e=B[n],z=B[v],H=[],L=s();try{if(!l(o[g("0x22")])||!l(o[g("0x2b")])||!l(o[g("0x1b")][g("0x36")][g("0x65")])||y[g("0x7a")]&&y[g("0x7a")]<=10){var c={};return c[g("0x43")]=!1,c[g("0x7f")]=g("0x69"),V(c)}}catch(x){}var i,C,b,G,U,Y=null;B[g("0x5f")]&&(i=function(){var x=g;d(x("0x5e")),P(x("0x68"))},C=2e4,b=s(),G=null,U=!1,function x(){var r=X;U||(s()-b>=C?i():G=o[r("0x18")](function(){x()},100))}(),Y=function(){U=!0,o[X("0x61")](G)}),function(n){var v=g;if(d(v("0x26")),y[v("0x34")]===v("0x4")||y[v("0x34")]!==v("0x11")&&!y[v("0x6a")][v("0x56")])d(v("0x52")),n();else try{y[v("0x22")]&&y[v("0x22")](v("0x2"),function x(){var r=v;d(r("0x4a")),y[r("0x57")](r("0x2"),x),n()})}catch(x){d(v("0x38")),P(x&&x[v("0x21")])}}(function(){var f=g;try{var a=s();d(f("0x4f")),S(e,function(x){var r=f;try{Y&&Y(),d(r("0x3c"));var n=o[r("0x58")](x[r("0x17")](r("0x72"))),v=x[r("0x23")][r("0x14")];if(!n||n<32e3||n!==v)return P(r("0x51"),n+r("0x15")+v),0;new o[r("0x1b")](x[r("0x23")])[r("0x6")](B[t]),B[t][r("0x1")](o,y,B)}catch(x){P(x&&x[r("0x19")])}},function(x,r){var n,v=f;try{Y&&Y(),d(v("0x54")),s()-a<2e3?z?S(z,function(){d(v("0x12")),P(x,r)},function(){var x=v,r={};r[x("0x43")]=!1,r[x("0x7f")]=x("0x69"),V(r)}):P(x,r):((n={})[v("0x43")]=!1,n[v("0x7f")]=v("0x69"),V(n))}catch(x){P(x&&x[v("0x19")])}})}catch(x){P(x&&x[f("0x21")])}})}else P(g("0x7b"));function d(x){var r=g,n={};n.d=x,n.t=s()-L,H[r("0x2f")](n)}function l(x){return typeof x===g("0x48")}function s(){var x=g;return o[x("0x58")](new o[x("0x39")])}function T(x,r,n){var v,f,a=g;try{o[a("0x8")]&&(n&&(f=g,v=(24e5*(s()/24e5)[f("0x30")]())[f("0x60")](36)[f("0x4c")](0,10),r=o[a("0x66")](v)+u+o[a("0x66")](function(x,r){for(var n=g,v=[],f=0;f<x[n("0x14")];f++){var a=x[n("0x81")](f)^r[n("0x81")](f%r[n("0x14")]);v[n("0x2f")](o[n("0xb")][n("0x1a")](a))}return v[n("0x2e")]("")}(r,v))),o[a("0x8")][a("0x6c")](x,r))}catch(x){}}function V(x){var r=g("0x2d");w&&w(x,r),T(a,r)}function K(x,r){var n=g;return r?x[n("0x28")](".")[n("0x4c")](-r)[n("0x2e")]("."):x}function P(x,r){var v=g,n=s(),f=new o[v("0x39")](n+36e5*B[A])[v("0x64")](),a=[],t=K(o[v("0x25")][v("0x6e")],2);function u(){for(var x=v,r=0;r<a[x("0x14")];r++){var n=a[r];y[x("0x2a")]=n[x("0x4b")]+"="+n[x("0x82")]+x("0x4d")+n[x("0x1c")]+x("0x71")+n[x("0xd")]+x("0x20")}}-1!==D[v("0x7e")](t)&&(t=K(o[v("0x25")][v("0x6e")],3));for(var w=B[N],e=0;e<w[v("0x14")];e++){var z={};z[v("0x4b")]=w[e],z[v("0x82")]=1,z[v("0x1c")]=f,z[v("0xd")]=t,a[v("0x2f")](z)}var L,c,i=function(x){for(var r=g,n=y[r("0x2a")][r("0x28")]("; "),v=0;v<n[r("0x14")];v++){var f=n[v][r("0x28")]("=");if(f[0]===x)return f[r("0x4c")](1)[r("0x2e")]("=")}return null}(M);i&&(y[v("0x2a")]=M+"="+i+v("0x71")+t+v("0x20"),(L={})[v("0x4b")]=M,L[v("0x82")]=i,L[v("0x1c")]=new o[v("0x39")](0)[v("0x64")](),L[v("0xd")]=t,a[v("0x2f")](L));try{o[v("0x22")](v("0x3a"),u),o[v("0x22")](v("0x31"),u)}catch(x){u()}try{(function(x,r){var n=g,v={};v[n("0x79")]=n("0x2d"),v[n("0x32")]=x,v[n("0x4e")]=r,v[n("0xa")]=o[n("0x76")]!==o[n("0x73")],v[n("0x53")]=H;var f={};f[n("0x80")]=n("0x63"),f[n("0x67")]=n("0x63"),f[n("0x7f")]=n("0x59"),f[n("0x7c")]=n("0x63"),f[n("0x32")]="i0",f[n("0x79")]="1";var a={};a[n("0x33")]=1;var t={};t[n("0x6f")]=v,t[n("0x3b")]=f,t[n("0x46")]=a,t[n("0x25")]=o[n("0x25")][n("0x6d")],t[n("0x1e")]=s(),t[n("0xc")]=n("0x5d"),t[n("0x74")]=n("0x37"),t[n("0x45")]=n("0x16"),t[n("0x82")]=1,t[n("0x79")]="1";var u=t,w=o[n("0x7d")][n("0x3")](u);{var e;l(o[n("0x62")][n("0x3e")])?o[n("0x62")][n("0x3e")](q,w):((e=new h)[n("0x3f")](n("0x24"),q,!0),e[n("0x5")](w))}})(x,r=r||""),T(j,[(new o[v("0x39")])[v("0x64")](),v("0x1f"),x,r,o[(c=g)("0x25")]?typeof o[c("0x25")][c("0x60")]===c("0x48")?o[c("0x25")][c("0x60")]():o[c("0x25")][c("0x6d")]||"":""][v("0x2e")]("\\n"),!0)}catch(x){}var C={};C[v("0x43")]=!0,C[v("0x7f")]=v("0x1f"),V(C)}function S(x,r,n){var v=g,f=new h;if(0!==f[v("0x34")])throw new Error(v("0x77"));if(f[v("0x3f")](v("0xe"),x,!0),1!==f[v("0x34")]||f[v("0x5")]!==h[v("0x36")][v("0x5")])throw new Error(v("0x5a"));f[v("0x5b")]=!0,f[v("0x29")]=function(){r(f)},f[v("0x84")]=function(){var x=v;n(x("0x44"),x("0x9")+f[x("0x0")])},f[v("0x5")]()}}(window,document,{
    src: "https://static-mon.yandex.net/static/main.js?pid=<PARTNER-ID>",
    cookie: "somecookie",
    callback: result_aab,
    debug: true
})}();
"""

# language=JavaScript
LUDCA_DECODE_SCRIPT = """
var ludka=localStorage.getItem('ludca');
if (ludka) {
    var base64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=",native64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";function addEquals(a){for(;0!=a.length%4;)a+="=";return a}function decodeBase64(a){a=decodeUInt8String(a);return utf8Decode(a)}
    function decodeUInt8String(a){var d=base64alphabet;if(-1!==a.indexOf("+")||-1!==a.indexOf("/"))d=native64alphabet;var b=[],c=0;for(a=a.replace(/[^A-Za-z0-9\-_=+\/]/g,"");c<a.length;){var e=d.indexOf(a.charAt(c++)),f=d.indexOf(a.charAt(c++)),g=d.indexOf(a.charAt(c++)),h=d.indexOf(a.charAt(c++)),k=(f&15)<<4|g>>2,l=(g&3)<<6|h;b.push(String.fromCharCode(e<<2|f>>4));64!==g&&b.push(String.fromCharCode(k));64!==h&&b.push(String.fromCharCode(l))}return b.join("")}
    function utf8Decode(a){for(var d=[],b=0;b<a.length;){var c=a.charCodeAt(b);if(128>c)d.push(String.fromCharCode(c)),b++;else if(191<c&&224>c){var e=a.charCodeAt(b+1);d.push(String.fromCharCode((c&31)<<6|e&63));b+=2}else{e=a.charCodeAt(b+1);var f=a.charCodeAt(b+2);d.push(String.fromCharCode((c&15)<<12|(e&63)<<6|f&63));b+=3}}return d.join("")}function xor(a,d){for(var b=[],c=0;c<a.length;c++){var e=a.charCodeAt(c)^d.charCodeAt(c%d.length);b.push(String.fromCharCode(e))}return b.join("")}
    function decode(a){var d=decodeUInt8String,b=a.split("dmVyc2lvbg"),c="bHVkY2E=";1!==b.length&&(c="bHVkY2E",d=decodeBase64,a=b[1]);b=a.split(c);a=addEquals(b[1]);b=decodeUInt8String(addEquals(b[0]));return xor(d(a),b)};
    return decode(ludka)
}
"""


AVAILABLE_ADBLOCKS_EXT_CHROME = [
    extensions.AdblockChromeInternal,
    extensions.AdblockPlusChromeInternal,
    extensions.AdguardChromeInternal,
    extensions.UblockOriginChromeInternal,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_FIREFOX = [
    extensions.AdblockFirefox,
    extensions.AdblockPlusFirefox,
    extensions.AdguardFirefox,
    extensions.UblockOriginFirefox,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_OPERA = [
    extensions.OperaExtensionInternal,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_YABRO = [
    extensions.AdblockChromeInternal,
    extensions.AdblockPlusChromeInternal,
    extensions.AdguardChromeInternal,
    extensions.UblockOriginChromeInternal,
    extensions.WithoutAdblock,
]

# TODO Chostery, Ublock, Adguard_beta, AdblockPlus_dev
