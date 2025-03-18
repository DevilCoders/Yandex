import {antiadbUrl} from 'app/lib/url';

describe('url', () => {
    test('antiadbUrl should return valid url', () => {
        const url = 'some.service.com/api/v3.0';
        expect(antiadbUrl(url)).toEqual(url);
    });
});
