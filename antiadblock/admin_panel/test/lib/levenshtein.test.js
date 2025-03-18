import levenshtein from 'app/lib/levenshtein';

describe('levenshtein', () => {
    describe('levenshtein', () => {
        const str2 = 'git';

        test('should return 1 distance between fit git (change)', () => {
            const str1 = 'fit';

            expect(levenshtein(str1, str2)).toEqual(1);
        });

        test('should return 1 distance between it git (add)', () => {
            const str1 = 'it';

            expect(levenshtein(str1, str2)).toEqual(1);
        });

        test('should return 1 distance between dgit git (del)', () => {
            const str1 = 'dgit';

            expect(levenshtein(str1, str2)).toEqual(1);
        });

        test('should return 0 distance between git git (equal)', () => {
            const str1 = 'git';

            expect(levenshtein(str1, str2)).toEqual(0);
        });

        test('should return 2 distance between giiit git (del symb into word)', () => {
            const str1 = 'giiit';

            expect(levenshtein(str1, str2)).toEqual(2);
        });

        test('should return 5s distance between giiit git (del symb before word and into)', () => {
            const str1 = 'sssgiiit';

            expect(levenshtein(str1, str2)).toEqual(5);
        });
    });
});
