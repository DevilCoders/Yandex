import i18n from 'app/lib/i18n';

// mock env and translation module
process.env.BEM_LANG = 'ru';
jest.mock('app/i18n/datetime/datetime.i18n/ru', () => ({
    datetime: {
        now: 'сейчас',
        tomorrow: 'завтра',
        days: ['день', 'дня', 'дней', 'дней']
    }
}));

jest.mock('app/i18n/datetime/datetime.i18n/en', () => ({
    datetime: {
        now: 'now',
        tomorrow: 'tomorrow'
    }
}));

describe('i18n', () => {
    describe('with BEM_LANG=en', () => {
        beforeAll(() => {
            process.env.BEM_LANG = 'en';
        });

        test('should return translation for word tomorrow', () => {
            expect(i18n('datetime', 'tomorrow')).toEqual('tomorrow');
        });

        test('should return translation for word now', () => {
            expect(i18n('datetime', 'now')).toEqual('now');
        });

        test('should ignore count parameter', () => {
            expect(i18n('datetime', 'now', 53)).toEqual('now');
        });

        test('should return key', () => {
            expect(i18n('datetime', 'yesterday123')).toEqual('yesterday123');
        });

        afterAll(() => {
            process.env.BEM_LANG = 'ru';
        });
    });

    describe('without count', () => {
        test('should return translation for word tomorrow', () => {
            expect(i18n('datetime', 'tomorrow')).toEqual('завтра');
        });

        test('should return translation for word now', () => {
            expect(i18n('datetime', 'now')).toEqual('сейчас');
        });

        test('should ignore count parameter', () => {
            expect(i18n('datetime', 'now', 53)).toEqual('сейчас');
        });

        test('should return key', () => {
            expect(i18n('datetime', 'yesterday')).toEqual('yesterday');
        });
    });

    describe('with count', () => {
        test('should return translation for count 0', () => {
            expect(i18n('datetime', 'days')).toEqual('дней');
        });

        test('should return translation for count 1', () => {
            expect(i18n('datetime', 'days', 1)).toEqual('день');
        });

        test('should return translation for count 2', () => {
            expect(i18n('datetime', 'days', 2)).toEqual('дня');
        });

        test('should return translation for count 5', () => {
            expect(i18n('datetime', 'days', 5)).toEqual('дней');
        });

        test('should return translation for count 11', () => {
            expect(i18n('datetime', 'days', 11)).toEqual('дней');
        });

        test('should return translation for count 15', () => {
            expect(i18n('datetime', 'days', 15)).toEqual('дней');
        });

        test('should return translation for count 24', () => {
            expect(i18n('datetime', 'days', 24)).toEqual('дня');
        });
    });
});
