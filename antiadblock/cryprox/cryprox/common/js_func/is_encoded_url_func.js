function (url) {
    if (url.indexOf("{seed}") === -1) {
        return false;
    }

    if ({is_encoded_url_regex}.test(url)){
        return true;
    }

    return false;
}
