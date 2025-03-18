export function isBetween(minDate, maxDate, selectedDay) {
    return !isNotBetween(minDate, maxDate, selectedDay);
}

export function isBetweenExclusive(startDate, endDate, day) {
    return startDate && endDate && day > startDate && day < endDate;
}

export function isNotBetween(minDate, maxDate, selectedDay) {
    return (minDate !== null && selectedDay.isBefore(minDate, 'day')) || (maxDate !== null && selectedDay.isAfter(maxDate, 'day'));
}

export function isWeekend(day) {
    return day.day() === 0 || day.day() === 6;
}

export function isoWeek(d) {
    d = new Date(Date.UTC(d.getFullYear(), d.getMonth(), d.getDate()));
    d.setUTCDate(d.getUTCDate() + 4 - (d.getUTCDay() || 7));
    const yearStart = new Date(Date.UTC(d.getUTCFullYear(), 0, 1));
    const msInDay = 86400000;

    return Math.ceil((((d - yearStart) / msInDay) + 1) / 7);
}

export function getValueOnInterVal(left, right, value) {
    let res = value;

    if (value < left) {
        res = left;
    } else if (right < value) {
        res = right;
    }

    return res;
}

export function dateDayJsIsEqual(date1, date2) {
    if (!date1 || !date2) {
        return false;
    }

    return (
        date1.date() === date2.date() &&
        date1.month() === date2.month() &&
        date1.year() === date2.year()
    );
}
