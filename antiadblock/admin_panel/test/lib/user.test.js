import {setUser, getUser} from 'app/lib/user';

describe('utils', () => {
    describe('user', () => {
        const testUser = {
            permissions: ['test_permission_1', 'test_permission_2'],
            user_id: 123456,
            yandexUid: '789000123'
        };

        // user its a global variable
        afterEach(() => {
            setUser(null);
        });

        test('setUser and getUser', () => {
            setUser(testUser);

            let savedUser = getUser();

            expect(savedUser.user_id).toBe(testUser.user_id);
            expect(savedUser.yandexUid).toBe(testUser.yandexUid);

            setUser(null);
            savedUser = getUser();
            expect(savedUser.user_id).not.toBeDefined();
            expect(savedUser.yandexUid).not.toBeDefined();
        });

        test('hasPermission', () => {
            setUser(testUser);

            const user = getUser();

            expect(user.hasPermission('test_permission_1')).toBeTruthy();

            expect(user.hasPermission('test_permission_3')).toBeFalsy();
        });
    });
});
