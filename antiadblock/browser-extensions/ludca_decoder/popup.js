// функция расшифровки
var base64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=",native64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";function addEquals(a){for(;0!=a.length%4;)a+="=";return a}function decodeBase64(a){a=decodeUInt8String(a);return utf8Decode(a)}
function decodeUInt8String(a){var d=base64alphabet;if(-1!==a.indexOf("+")||-1!==a.indexOf("/"))d=native64alphabet;var b=[],c=0;for(a=a.replace(/[^A-Za-z0-9\-_=+\/]/g,"");c<a.length;){var e=d.indexOf(a.charAt(c++)),f=d.indexOf(a.charAt(c++)),g=d.indexOf(a.charAt(c++)),h=d.indexOf(a.charAt(c++)),k=(f&15)<<4|g>>2,l=(g&3)<<6|h;b.push(String.fromCharCode(e<<2|f>>4));64!==g&&b.push(String.fromCharCode(k));64!==h&&b.push(String.fromCharCode(l))}return b.join("")}
function utf8Decode(a){for(var d=[],b=0;b<a.length;){var c=a.charCodeAt(b);if(128>c)d.push(String.fromCharCode(c)),b++;else if(191<c&&224>c){var e=a.charCodeAt(b+1);d.push(String.fromCharCode((c&31)<<6|e&63));b+=2}else{e=a.charCodeAt(b+1);var f=a.charCodeAt(b+2);d.push(String.fromCharCode((c&15)<<12|(e&63)<<6|f&63));b+=3}}return d.join("")}function xor(a,d){for(var b=[],c=0;c<a.length;c++){var e=a.charCodeAt(c)^d.charCodeAt(c%d.length);b.push(String.fromCharCode(e))}return b.join("")}
function decode(a){var d=decodeUInt8String,b=a.split("dmVyc2lvbg"),c="bHVkY2E=";1!==b.length&&(c="bHVkY2E",d=decodeBase64,a=b[1]);b=a.split(c);a=addEquals(b[1]);b=decodeUInt8String(addEquals(b[0]));return xor(d(a),b)};

// запись результата
function printResult(text) {
    var resultField = document.getElementById('result');
    resultField.value = text;
}

// расшифровка людки с текущей страницы
function decodeLocalLudca(){
    chrome.tabs.getSelected(null, function(tab){
        chrome.tabs.executeScript(tab.id, {code: "localStorage.getItem('ludca')"}, function(response) {
            var decoded = 'No ludka';
            if (response && response[0] != null){
                decoded = decode(String(response));
            }
            printResult(decoded);
        });
    });
}

document.addEventListener('DOMContentLoaded', function() {
    var checkLocalButton = document.getElementById('checkLocal');
    checkLocalButton.addEventListener('click', function() {
        decodeLocalLudca()
    }, false);

    var checkRemoteButton = document.getElementById('checkRemote');
    checkRemoteButton.addEventListener('click', function() {
        var text = document.getElementById('ludcaRemote').value;
        var decoded = 'No ludka';
        if (text){
            decoded = decode(text);
        }
        printResult(decoded);
    }, false);
}, false);

// сразу при открытии попапа расшифровываем людку в данной странице
decodeLocalLudca();
