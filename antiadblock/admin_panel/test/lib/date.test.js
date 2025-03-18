import {timeAgo, getMonthNamesForLocale, formatDateTo} from 'app/lib/date';

// mock env and translation module
process.env.BEM_LANG = 'ru';
jest.mock('app/i18n/datetime/datetime.i18n/ru', () => ({
    datetime: {
        ago: 'назад',
        'less-than': 'меньше',
        day: ['день', 'дня', 'дней', 'дней'],
        month: ['месяц', 'месяца', 'месяцев', 'месяцев'],
        hour: ['час', 'часа', 'часов', 'часов'],
        second: ['секунду', 'секунды', 'секунд', 'секунд'],
        year: ['год', 'года', 'лет', 'лет']
    }
}));

describe('date', () => {
    describe('timeAgo', () => {
        test('should return less than a second ago', () => {
            const now = new Date();
            expect(timeAgo(now)).toMatchSnapshot();
        });

        test('should return 5 months ago', () => {
            const now = new Date();
            now.setMonth(now.getMonth() - 5);
            expect(timeAgo(now)).toMatchSnapshot();
        });

        test('should return 5 years ago', () => {
            const now = new Date();
            now.setFullYear(now.getFullYear() - 5);
            expect(timeAgo(now)).toMatchSnapshot();
        });

        test('should return 1 day ago', () => {
            const now = new Date();
            now.setHours(now.getHours() - 35);
            expect(timeAgo(now)).toMatchSnapshot();
        });
    });

    describe('formatDateTo', () => {
        test('set invalid date will return \'Invalid Date\'', () => {
            const time = 'asdadsa';
            const format = 'DD.MM.YYYY HH:mm:ss';
            const result = 'Invalid Date';

            expect(formatDateTo(time, format)).toEqual(result);
        });

        test('set date will return in format \'DD.MM.YYYY HH:mm:ss\'', () => {
            const time = '2019-09-11T15:22:54';
            const format = 'DD.MM.YYYY HH:mm:ss';
            const result = '11.09.2019 15:22:54';

            expect(formatDateTo(time, format)).toEqual(result);
        });
    });

    describe('getMonthNamesForLocale', () => {
        test('test en locale', () => {
            expect(getMonthNamesForLocale('en')).toEqual(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']);
        });
    });
});
