import i18n from 'app/lib/i18n';

export default class ResponseError extends Error {
    constructor(status, response) {
        super();
        const defaultError = i18n('common', 'http-request-error');
        const isDefaultError = !status || status === 500;

        this.message = isDefaultError ?
                            defaultError :
                            response && response.message ?
                                response.message :
                                defaultError;

        this.name = 'ResponseError';
        this.response = response;
        this.status = status;
    }
}
