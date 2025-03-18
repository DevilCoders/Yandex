function (className) {
    const base64alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=';

    function decodeUInt8String(input) {
        const output = [];
        let i = 0;

    
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

    function encodeUInt8String(input) {
        let output = '';
        let i = 0;
    
        while (i < input.length) {
            const chr1 = input.charCodeAt(i++);
            const chr2 = input.charCodeAt(i++);
            const chr3 = input.charCodeAt(i++);
    
            const enc1 = chr1 >> 2;
            const enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
            let enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
            let enc4 = chr3 & 63;
    
            if (isNaN(chr2)) {
                enc3 = enc4 = 64;
            } else if (isNaN(chr3)) {
                enc4 = 64;
            }
    
            output =
                output +
                base64alphabet.charAt(enc1) +
                base64alphabet.charAt(enc2) +
                base64alphabet.charAt(enc3) +
                base64alphabet.charAt(enc4);
        }
    
        return output;
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

    function cropEquals (base64) {
    return base64.replace(/=+$/, '');
    }

    function encode (url) {
        const xoredUrl = xor(url, getKey());
        return cropEquals(encodeUInt8String(xoredUrl));
    }

    const encoded = encode(className);
    const CHAR_CODE_A = 'a'.charCodeAt(0);
    const firstChar = String.fromCharCode(getKey().charCodeAt(0) % 26 + CHAR_CODE_A);

    return (firstChar + encoded).replace(/-/g, 'a');
}