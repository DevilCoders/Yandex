import redirect from 'app/lib/redirect';

describe('utils', () => {
    describe('redirect', () => {
        const initialUrl = 'some url';

        /*
            В jest >23.6.x есть изменения в результате которых падал этот тест
            https://github.com/facebook/jest/issues/890
         */
        beforeAll(() => {
            const location = JSON.stringify(window.location);
            delete window.location;

            Object.defineProperty(window, 'location', {
                value: JSON.parse(location)
            });

            Object.defineProperty(window.location, 'href', {
                value: initialUrl,
                configurable: true
            });
        });

        it('should change location.href', () => {
            expect(location.href).toBe(initialUrl);

            const testValue = 'new test url';

            redirect(testValue);

            expect(location.href).toBe(testValue);
        });
    });
});
