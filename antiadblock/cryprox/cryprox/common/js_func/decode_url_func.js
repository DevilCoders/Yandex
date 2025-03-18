function (url) {
    const base64alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=';

    function decodeUInt8String(input) {
        const output = [];
        let i = 0;

        input = input.replace(/[^A-Za-z0-9\-_=]/g, '');

        while (i < input.length) {
            const enc1 = base64alphabet.indexOf(input.charAt(i++));
            const enc2 = base64alphabet.indexOf(input.charAt(i++));
            const enc3 = base64alphabet.indexOf(input.charAt(i++));
            const enc4 = base64alphabet.indexOf(input.charAt(i++));

            const chr1 = (enc1 << 2) | (enc2 >> 4);
            const chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
            const chr3 = ((enc3 & 3) << 6) | enc4;

            output.push(String.fromCharCode(chr1));

            if (enc3 !== 64) {
                output.push(String.fromCharCode(chr2));
            }
            if (enc4 !== 64) {
                output.push(String.fromCharCode(chr3));
            }
        }

        return output.join('');
    }

    function addEquals(base64) {
        while (base64.length % 4 !== 0) {
            base64 += '=';
        }

        return base64;
    }


    function getKey () {
        return decodeUInt8String("{encode_key}"); // placeholder
    }

    function xor (data, key) {
        const result = [];

        for (let i = 0; i < data.length; i++) {
            const xored = data.charCodeAt(i) ^ key.charCodeAt(i % key.length);

            result.push(String.fromCharCode(xored));
        }

        return result.join('');
    }

    function decode (encodedUrl) {
        const fullUrl = addEquals(encodedUrl);

        return xor(decodeUInt8String(fullUrl), getKey());
    }

    function isEncodedUrl (url) {
        if (url.indexOf("{seed}") === -1) {
            return false;
        }

        if ({is_encoded_url_regex}.test(url)){
            return true;
        }

        return false;
    }

    if (!isEncodedUrl(url)) {return url;}

    const lastSymbolInUrl = url.slice(-1);
    let encodedUrl = url;

    if ({trailing_slash} && lastSymbolInUrl === "/") { // placeholder
        encodedUrl = encodedUrl.slice(0, -1);
    }

    encodedUrl = encodedUrl.split("{seed}")[1]; // placeholder
    encodedUrl = addEquals(encodedUrl.replace(/[\/?.=&!$()|*~\[\]^<>\\]/g, ""));

    return decode(encodedUrl).replace(/\s[a-zA-Z]*$/, "").replace(/__AAB_ORIGIN.*?__/, "");
}
