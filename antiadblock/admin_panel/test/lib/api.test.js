import {antiadbRequest} from 'app/lib/api';

describe('api', () => {
    describe('request', () => {
        test('simple request', () => {
            fetch.mockResponseOnce(JSON.stringify({}));

            return antiadbRequest({}).then(response => {
                expect(response).toEqual({});
            });
        });

        test('simple request with non json response', () => {
            fetch.mockResponseOnce('text response');

            return antiadbRequest({
                options: {json: false}
            }).then(response => {
                expect(response).toEqual('text response');
            });
        });

        test('simple request with unexpected non json response', () => {
            fetch.mockResponseOnce('text response');

            return antiadbRequest({}).catch(error => {
                expect(error).toBeInstanceOf(Error);
            });
        });

        test('simple failed request', () => {
            fetch.mockRejectOnce();

            return antiadbRequest({}).catch(error => {
                expect(error).toBeInstanceOf(Error);
            });
        });

        test('simple failed with error response', () => {
            fetch.mockResponseOnce(
                JSON.stringify({}),
                {
                    status: 400
                }
            );

            return antiadbRequest({}).catch(error => {
                expect(error).toBeInstanceOf(Error);
                expect(error.status).toEqual(400);
                expect(error.response).toEqual({});
            });
        });

        test('simple failed with non json error response', () => {
            fetch.mockResponseOnce(
                'string error',
                {
                    status: 400
                }
            );

            return antiadbRequest({}).catch(error => {
                expect(error).toBeInstanceOf(Error);
                expect(error.status).toEqual(400);
                expect(error.response).toBeUndefined();
            });
        });
    });
});
