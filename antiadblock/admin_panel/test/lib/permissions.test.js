import {setPermissions, getPermissions} from 'app/lib/permissions';

describe('utils', () => {
    describe('permissions', () => {
        const testPermissions = {
            TEST_PERMISSION_1: 'test_permission_1',
            TEST_PERMISSION_2: 'test_permission_2'
        };

        // permissions its a global variable
        afterEach(() => {
            setPermissions(null);
        });

        test('setPermissions and getPermissions', () => {
            setPermissions(testPermissions);

            let savedPermissions = getPermissions();

            expect(savedPermissions).toEqual(testPermissions);

            setPermissions(null);
            savedPermissions = getPermissions();
            expect(savedPermissions).toBe(null);
        });
    });
});
