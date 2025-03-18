module.exports = function(config) {
    config.paths.root = './app/i18n';

    config.tanker.force = true;
    config.tanker.project = 'antiadb';
    config.tanker.host = 'tanker-api.yandex-team.ru';
};
