function format_bytes(bytes, decimals) {
    if (bytes == 0 || bytes == undefined) return '0 B';
    var k = 1024,
        dm = decimals || 2,
        sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'],
        i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
}


function format_limits(guarantee, limit, bytes) {
    var result = '';
    if (guarantee != undefined && guarantee != '') {
        if (bytes || bytes == undefined) {
            guarantee = format_bytes(guarantee);
        }
        result += '<span class="label label-success">' +
                  guarantee + '</span> - ';
    }
    if (limit != undefined && limit != '') {
        if (bytes || bytes == undefined) {
            limit = format_bytes(limit);
        }
        result += '<span class="label label-warning">' +
                  limit + '</span>';
    }
    return result;
}


function format_deploy(json_resp) {
    var result = '';
    if (json_resp['deploy'] != undefined) {
        if (json_resp['deploy']['deploy_version'] == 2) {
            id = json_resp['deploy']['deploy_id'];
            deploy_api_base = json_resp['deploy']['deploy_api_url'];
            deploy_env = json_resp['deploy']['deploy_env'];
            result += 'You can check shipment by shell command:</br><b>' +
                'mdb-admin -d ' + deploy_env + ' deploy shipments get ' + id +
                '<b></br></br>';
            deploy_url = deploy_api_base + 'shipments/' + id;
        } else {
            deploy_url = 'https://salt-db.mail.yandex-team.ru/ship/' +
                        json_resp['deploy']['deploy_id'];
        }
        result += 'Deploy: <a href="' + deploy_url +
                  '" class="alert-link">' + deploy_url + '</a>';
    }
    if (json_resp['transfer'] != undefined) {
        result += 'Transfer initiated: ' + json_resp['transfer'];
    }
    return result;
}
