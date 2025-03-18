import ResponseError from 'app/errors/response-error';

const getRequestConfiguration = () => {
    return {
        queryParams: {}
    };
};

export default function request(prefix, requestObject) {
    const reqQueryParams = requestObject.queryParams || {};
    const prepQueryParams = Object
        .keys(reqQueryParams)
        .reduce((queryParams, key) => {
            if (reqQueryParams[key] !== undefined) {
                queryParams[key] = reqQueryParams[key];
            }
            return queryParams;
    }, {});
    const queryParams = Object.assign({},
        prepQueryParams,
        getRequestConfiguration().queryParams
    );
    let {options, path} = requestObject,
        params;

    options = options || {};

    // Можно передать массив в queryParams, тогда он должен склеиться в param=1&param=2&param=3 итд
    params = Object.keys(queryParams).map(function(paramName) {
        const paramValue = queryParams[paramName];

        if (Array.isArray(paramValue)) {
            return queryParams[paramName].map(paramItemValue => `${paramName}=${paramItemValue}`).join('&');
        }

        return `${paramName}=${encodeURIComponent(paramValue)}`;
    }).join('&');

    options.headers = options.headers || {};

    if (options.body && !(options.body instanceof FormData)) {
        options.headers['Content-Type'] = 'application/json';
        options.body = JSON.stringify(options.body);
    }

    return fetch(prefix + path + '?' + params, Object.assign({}, {
        credentials: 'same-origin'
    }, options)).then(function(response) {
        if (response.ok) {
            if (options.json !== false) {
                return response
                    .json()
                    .catch(function() {
                        return Promise.reject(new ResponseError(response.status, response));
                    });
            }

            return response.text();
        }

        return response.json()
            .then(function(json) {
                return Promise.reject(new ResponseError(response.status, json));
            }, function() {
                return Promise.reject(new ResponseError(response.status));
            });
    }, function(error) {
        return Promise.reject(new ResponseError(null, error));
    });
}

export function antiadbRequest(requestObject, countRetry = 0) {
    return request('/rest', requestObject).then(data => {
        return data;
    }, err => {
        return countRetry > 0 ?
            antiadbRequest(requestObject, countRetry--) :
            Promise.reject(err);
    });
}
