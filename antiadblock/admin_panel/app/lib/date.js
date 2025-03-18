import i18n from 'app/lib/i18n';
import dayjs from 'dayjs';
import 'dayjs/locale/ru';
import weekOfYear from 'dayjs/plugin/weekOfYear';

dayjs.locale('ru');
dayjs.extend(weekOfYear);

export function getMonthNamesForLocale(locale) {
    const format = new Intl.DateTimeFormat(locale, {month: 'short'});
    const months = [];
    for (let month = 0; month < 12; month++) {
        const testDate = new Date(Date.UTC(2000, month, 1, 0, 0, 0));
        months.push(format.format(testDate));
    }
    return months;
}

export function formatDateTo(dateString, format) {
    return dayjs(dateString).format(format);
}

export function timeAgo(date) {
    const now = new Date();
    const human = ['second', 'minute', 'hour', 'day', 'week', 'month', 'year'];
    const delta = [1000, 60, 60, 24, 7, 4, 12];

    let diffrence = Math.abs(date.getTime() - now.getTime());

    for (let i = 0; i < delta.length; i++) {
        diffrence /= delta[i];

        // less than
        if (diffrence < 1) {
            return `${i18n('datetime', 'less-than')} ${i18n('datetime', human[i], 2)} ${i18n('datetime', 'ago')}`;
        }

        if (i === delta.length - 1 || delta[i + 1] > diffrence) {
            const int = Math.round(diffrence);
            return `${int} ${i18n('datetime', human[i], int)} ${i18n('datetime', 'ago')}`;
        }
    }
}

export {
    dayjs
};

