function (url) {
    const seed = "{seed}"; // placeholder

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

    function repeat(str, count) {
        let result = [];
        for (let i = 0; i < count; i++) {
            result.push(str);
        }

        return result.join('');
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

        return result.join("");
    }

    function cropEquals (base64) {
        return base64.replace(/=+$/, "");
    }

    function encode (url) {
        const xoredUrl = xor(url, getKey());
        return cropEquals(encodeUInt8String(xoredUrl));
    }

    let sdr = {seedForMyRandom};

    function getRandomInt (min, max) {
        sdr = (sdr * 73) % 9949;
        let rand = min + (sdr / 9950) * (max + 1 - min);
        rand = Math.floor(rand);
        return rand;
    }

    function getRandomChar (from, to) {
        from = from || "a";
        to = to || "z";
        const charCode = getRandomInt(from.charCodeAt(0), to.charCodeAt(0));

        return String.fromCharCode(charCode);
    }

    function mixWithSlashes (encodedUrl) {
        const CHUNK_LENGTH = 7;
        const urlParts = encodedUrl.match(/.{1,7}/g);

        if (urlParts === null) {
            return encodedUrl;
        }
        let urlTemplate = {urlTemplate};
        let symbolsToInsert = '';
        let symbolsToInsertSet = '';
        for (let i = 0; i < urlTemplate.length; i++) {
            symbolsToInsert += repeat(urlTemplate[i][0], urlTemplate[i][1]);
            symbolsToInsertSet += urlTemplate[i][0];
        }
        for (let i = 0; i < urlParts.length - 1 && i < symbolsToInsert.length; i++) {
            const positionToInsert = getRandomInt(1, CHUNK_LENGTH - 1);
            const part = urlParts[i].split("");

            part.splice(positionToInsert, 0, symbolsToInsert[i]);
            urlParts[i] = part.join("");
        }
        var url = urlParts.join("");
        if (symbolsToInsertSet.indexOf('&') !== -1) {
            return url.replace(/&(lt|gt)/ig, '$1');
        }
        else{
            return url
        }
    }

    function cryptNumber (num, minEncryptedLength) {
        const k = seed.charCodeAt(0);
        const b = seed.charCodeAt(5);
        const result = (num * k + b).toString().split("");

        while (result.length < minEncryptedLength) {
            const positionToInsert = getRandomInt(0, result.length);
            const charToInsert = getRandomChar();

            result.splice(positionToInsert, 0, charToInsert);
        }

        result.push("/");
        return result.join("");
    }

    function expandUrl(url, minLengthDecoded) {  // minLengthDecoded - это длина расшифрованного урла, до нее добиваем и при шифровке урл станет нужной длины
        if (minLengthDecoded > url.length) {
            let symbolsToGenerate = Math.max(minLengthDecoded - url.length - 1, 0);  // 1-пробел который мы вставим разделителем

            let randomSymbols = "";
            for (let i = 0; i < symbolsToGenerate; i++) {
                randomSymbols += getRandomChar();
            }

            return url + " " + randomSymbols;
        }
        return url;
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

    if (isEncodedUrl(url)) {return url;}

    let expandedUrl = url + "__AAB_ORIGIN{aab_origin}__";
    let url_is_subdocument = false;
    if (/^(?:https?:)?\/\/yastat(?:ic)?\.net\/safeframe-bundles\/[\d.\-\/]*\/(?:protected\/)?render(?:_adb)?\.html$/.test(url)){
        expandedUrl = expandUrl(expandedUrl, {crypted_subdocument_url_min_length});  // placeholder
        url_is_subdocument = true;
    } else if (!/^(?:https?:)?\/\/cryprox\.yandex\.net\/bamboozled/.test(url)) {
        expandedUrl = expandUrl(expandedUrl, {min_length_decoded});  // placeholder
    }
    const encoded = encode(expandedUrl);
    const encodedLink = mixWithSlashes(encoded);

    const linkLength = encoded.length;
    let url_prefix = "{url_prefix}"; // placeholder

    // Под бескуковый домен шифруем заданные явно ссылки
    if (/{client_cookieless_regex}/.test(url)) {
        url_prefix = "{cookieless_url_prefix}";  // placeholder
    }
    if (url.indexOf('//') === 0) {
        url_prefix = url_prefix.replace(/^https?:/, ''); // https://st.yandex-team.ru/ANTIADB-641
    }
    const prefix = url_prefix + cryptNumber(linkLength, 9) + seed;
    const lastSlash = {trailing_slash} ? "/" : "";
    const result = prefix + encodedLink + lastSlash;
    if ({subdocument_url_is_relative} && url_is_subdocument){
        // creating relative url by replacing groups (https:)(//)(domain.com/)uripath/.... with '', '', '/'
        return result.replace(/^https?:/, '').replace(/(^\/\/)/, '').replace(/(.*?\/)/, '/');
    } else {
        return result;
    }
}
