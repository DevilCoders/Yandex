// Список сервисов для получения метрик
export const METRICS = {
    HTTP_CODES: 'http_codes',
    ERRORS_BY_DOMAINS: 'http_errors_domain_type',
    BAMBOOZLED_BY_APP: 'bamboozled_by_app',
    BAMBOOZLED_BY_BRO: 'bamboozled_by_bro',
    TIMINGS: 'fetch_timings',
    PROPORTIONS: 'adblock_apps_proportions'
};

// Маппинг данных с сервера на имена графиков
export const GRAPHS = {
    status: METRICS.HTTP_CODES,
    percents: METRICS.HTTP_CODES,
    domains: METRICS.ERRORS_BY_DOMAINS,
    blockers: METRICS.BAMBOOZLED_BY_APP,
    browsers: METRICS.BAMBOOZLED_BY_BRO,
    timings: METRICS.TIMINGS,
    proportions: METRICS.PROPORTIONS
};

// Доступные для выбора ranges. Значения в секундах
export const RANGE = {
    '15m': 15,
    '30m': 30,
    '1h': 60,
    '4h': 240,
    '12h': 720,
    '24h': 1440,
    '4d': 5760,
    '7d': 10080
};

// Фискированные цвета для отображения данных
export const COLORS = {
    total: '#000000',
    200: '#7cb5ec',
    500: '#f45b5b'
};
