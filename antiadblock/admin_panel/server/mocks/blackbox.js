/**
 * Mock for express-uatraits middleware
 * @see https://github.yandex-team.ru/project-stub/express-blackbox
 */
module.exports = function() {
    return function(req, res, next) {
        req.blackbox = {
            age: 577,
            ttl: '5',
            session_fraud: 0,
            error: 'OK',
            status: 'VALID',
            uid: '204409962',
            login: 'bayan-test-acm',
            have_password: true,
            have_hint: true,
            karma: {
                value: 0
            },
            karma_status: {
                value: 0
            },
            auth: {
                password_verification_age: 4982,
                have_password: true,
                secure: false,
                allow_plain_text: true,
                partner_pdd_token: false
            },
            avatar: {
                default: '0/0-0'
            }
        };
        next();
    };
};
