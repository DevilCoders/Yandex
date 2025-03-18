import {scroll} from 'app/lib/animation';

describe('utils', () => {
    describe('animation', () => {
        const mockElement = {
            addEventListener: jest.fn(),
            removeEventListener: jest.fn(),
            scrollTop: 500
        };
        const SCROLL_OFFSET = 30;
        const SCROLL_PERIOD = 10;

        jest.useFakeTimers();

        beforeEach(() => {
            setInterval.mockClear();
            mockElement.addEventListener.mockClear();
            mockElement.removeEventListener.mockClear();
        });

        test('main logic', () => {
            const delay = SCROLL_PERIOD;
            const offset = 100;
            const cb = jest.fn();

            scroll(mockElement, offset, delay, cb);
            expect(setInterval.mock.calls).toHaveLength(1);
            expect(mockElement.addEventListener.mock.calls).toHaveLength(1);
            expect(mockElement.addEventListener.mock.calls[0][0]).toBe('wheel');
            expect(cb).not.toBeCalled();

            jest.runAllTimers();

            expect(cb).toBeCalled();
            expect(mockElement.scrollTop + SCROLL_OFFSET).toBe(offset);
            expect(mockElement.removeEventListener.mock.calls).toHaveLength(1);
            expect(mockElement.removeEventListener.mock.calls[0][0]).toBe('wheel');
        });

        test('without callback', () => {
            const delay = SCROLL_PERIOD;
            const offset = 100;

            scroll(mockElement, offset, delay);
            jest.runAllTimers();

            expect(mockElement.scrollTop + SCROLL_OFFSET).toBe(offset);
            expect(mockElement.removeEventListener.mock.calls).toHaveLength(1);
            expect(mockElement.removeEventListener.mock.calls[0][0]).toBe('wheel');
        });

        test('call multiple times', () => {
            const delay = SCROLL_PERIOD;
            const offset = 100;
            const cb = jest.fn();

            scroll(mockElement, offset, delay, cb);
            scroll(mockElement, offset, delay, cb);
            scroll(mockElement, offset, delay, cb);
            scroll(mockElement, offset, delay, cb);

            expect(setInterval.mock.calls).toHaveLength(4);
            expect(mockElement.addEventListener.mock.calls).toHaveLength(4);
            expect(mockElement.removeEventListener.mock.calls).toHaveLength(3);

            jest.runAllTimers();
            jest.runAllTimers();
            jest.runAllTimers();
            jest.runAllTimers();
            expect(mockElement.removeEventListener.mock.calls).toHaveLength(4);
            expect(cb.mock.calls).toHaveLength(4);
        });

        test('call with big delay', () => {
            const delay = 100;
            const offset = 100;
            const cb = jest.fn();

            scroll(mockElement, offset, delay, cb);

            let prevDistance = mockElement.scrollTop - offset;

            const count = (delay / SCROLL_PERIOD) - 1;
            for (let i = 0; i < count; i++) {
                jest.runAllTimers();

                const currentDistance = mockElement.scrollTop - offset;
                expect(currentDistance).toBeLessThanOrEqual(prevDistance);
                prevDistance = currentDistance;
            }

            expect(mockElement.scrollTop + SCROLL_OFFSET).toBe(offset);
            expect(setInterval.mock.calls).toHaveLength(1);
            expect(mockElement.addEventListener.mock.calls).toHaveLength(1);
            expect(mockElement.removeEventListener.mock.calls).toHaveLength(1);
            expect(cb.mock.calls).toHaveLength(1);
        });
    });
});
