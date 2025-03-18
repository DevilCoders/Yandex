const PREFIX = ''


class ApiError extends Error {
    constructor(url, status, error) {
        super();
        this.message = `Ошибка при запросе к API. Url: ${url}. Код: ${status}. Причина: ${error}`;
    }
}

function _request(method, url, body = undefined, prefix = PREFIX) {
    let headers = {};
    if (body && typeof(body) === 'object' && !('append' in body)) {
        body = JSON.stringify(body);
        headers = { 'Content-Type': 'application/json' };
    }
    const request = new Request(prefix + url, {
        method: method,
        body,
        headers,
    });
    return fetch(request)
        .then(response => {
            if (!response.ok) {
                throw new ApiError(url, response.status, response.statusText);
            }
            if (response.status === 204)
                return [];
            return response.json();
        })
        .catch(error => {
            throw new ApiError(url, 500, `Сервер не вернул JSON. Ошибка: ${error}`);
        });
}

async function get(url, prefix) {
    return _request('GET', url, undefined, prefix);
}

async function post(...options) {
    return _request('POST', ...options);
}

export default { get, post };
