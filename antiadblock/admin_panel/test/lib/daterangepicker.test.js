import * as dates from 'app/lib/daterangepicker';
import {dayjs} from 'app/lib/date';

describe('daterangepicker', () => {
    describe('isWeekend', () => {
        // 0 - воскресенье
        // 6 - суббота
        // [1-5] - будние дни
        const dayMock = value => ({
            day: () => (value)
        });

        test('test return is weekend', () => {
            expect(dates.isWeekend(dayMock(0))).toEqual(true);
        });

        test('test return is not weekend', () => {
            expect(dates.isWeekend(dayMock(4))).toEqual(false);
        });

        test('test return is weekend', () => {
            expect(dates.isWeekend(dayMock(6))).toEqual(true);
        });
    });

    describe('isBetweenExclusive', () => {
        // check in range dates
        const startDate = dayjs([2019, 7, 1]); // 01.07.2019
        const endDate = dayjs([2019, 7, 10]); // 10.07.2019

        test('test return date in range', () => {
            expect(dates.isBetweenExclusive(startDate, endDate, dayjs([2019, 7, 6]))).toEqual(true);
        });

        test('test return date > endDate in range', () => {
            expect(dates.isBetweenExclusive(startDate, endDate, dayjs([2019, 7, 11]))).toEqual(false);
        });

        test('test return date < startDate in range', () => {
            expect(dates.isBetweenExclusive(startDate, endDate, dayjs([2019, 6, 20]))).toEqual(false);
        });
    });

    describe('isNotBetween', () => {
        // check disable dates
        const minDate = dayjs([2019, 7, 1]); // 01.07.2019
        const maxDate = dayjs([2019, 7, 10]); // 10.07.2019

        test('test return date maxDate is between [maxDate, minDate]', () => {
            expect(dates.isNotBetween(minDate, maxDate, dayjs([2019, 7, 6]))).toEqual(false);
        });

        test('test return date === maxDate is between [maxDate, minDate]', () => {
            expect(dates.isNotBetween(minDate, maxDate, dayjs(maxDate))).toEqual(false);
        });

        test('test return date === minDate is between [maxDate, minDate]', () => {
            expect(dates.isNotBetween(minDate, maxDate, dayjs(minDate))).toEqual(false);
        });

        test('test return date > maxDate is not between [maxDate, minDate]', () => {
            expect(dates.isNotBetween(minDate, maxDate, dayjs([2019, 7, 11]))).toEqual(true);
        });

        test('test return date < minDate is not between [maxDate, minDate]', () => {
            expect(dates.isNotBetween(minDate, maxDate, dayjs([2019, 6, 20]))).toEqual(true);
        });
    });

    describe('isBetween', () => {
        // check disable dates
        const minDate = dayjs([2019, 7, 1]); // 01.07.2019
        const maxDate = dayjs([2019, 7, 10]); // 10.07.2019

        test('test return date maxDate is between [maxDate, minDate]', () => {
            expect(dates.isBetween(minDate, maxDate, dayjs([2019, 7, 6]))).toEqual(true);
        });

        test('test return date === maxDate is between [maxDate, minDate]', () => {
            expect(dates.isBetween(minDate, maxDate, dayjs(maxDate))).toEqual(true);
        });

        test('test return date === minDate is between [maxDate, minDate]', () => {
            expect(dates.isBetween(minDate, maxDate, dayjs(minDate))).toEqual(true);
        });

        test('test return date > maxDate is not between [maxDate, minDate]', () => {
            expect(dates.isBetween(minDate, maxDate, dayjs([2019, 7, 11]))).toEqual(false);
        });

        test('test return date < minDate is not between [maxDate, minDate]', () => {
            expect(dates.isBetween(minDate, maxDate, dayjs([2019, 6, 20]))).toEqual(false);
        });
    });

    describe('isoWeek', () => {
        test('test return number of week in ISO format 01.01.2019 (1 week)', () => {
            expect(dates.isoWeek(dayjs([2019, 1, 1]).toDate())).toEqual(1);
        });

        test('test return number of week in ISO format 29.12.2019 (52 week)', () => {
            expect(dates.isoWeek(dayjs([2019, 12, 29]).toDate())).toEqual(52);
        });

        test('test return number of week in ISO format 29.12.2018 (52 week)', () => {
            expect(dates.isoWeek(dayjs([2019, 12, 29]).toDate())).toEqual(52);
        });
    });

    describe('dateDayJsIsEqual', () => {
        test('test return not equal dates', () => {
            expect(dates.dateDayJsIsEqual(dayjs([2019, 1, 1]), dayjs([2019, 1, 2]))).toEqual(false);
        });

        test('test return equal dates', () => {
            expect(dates.dateDayJsIsEqual(dayjs([2019, 1, 1]), dayjs([2019, 1, 1]))).toEqual(true);
        });

        test('test return not equal without one of param', () => {
            expect(dates.dateDayJsIsEqual(dayjs([2019, 12, 29]))).toEqual(false);
        });
    });

    describe('getValueOnInterVal', () => {
        const mockFirstDate = 4;
        const mockSecondDate = 10;

        test('test [first, second] < value return second param', () => {
            const value = 11;

            expect(dates.getValueOnInterVal(mockFirstDate, mockSecondDate, value)).toEqual(mockSecondDate);
        });

        test('test value < [first, second] return first param', () => {
            const value = 2;

            expect(dates.getValueOnInterVal(mockFirstDate, mockSecondDate, value)).toEqual(mockFirstDate);
        });

        test('test [first < value < second] return value', () => {
            const value = 5;

            expect(dates.getValueOnInterVal(mockFirstDate, mockSecondDate, value)).toEqual(value);
        });
    });
});
