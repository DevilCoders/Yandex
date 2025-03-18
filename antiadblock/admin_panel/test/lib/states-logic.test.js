import {prepareStates} from 'app/lib/states-logic';

describe('states-logic', () => {
    describe('prepareStates', () => {
        const stateItems = {
            autoru: [
                {
                    in_progress: false,
                    name: 'bamboozled_adblock',
                    outdated: 0,
                    state: 'green',
                    transition_time: 1551958994
                },
                {
                    in_progress: false,
                    name: 'bamboozled_adguard',
                    outdated: 0,
                    state: 'red',
                    transition_time: 1551958980
                }
            ],
            echomsk: [
                {
                    in_progress: false,
                    name: 'bamboozled_adblock',
                    outdated: 1,
                    state: 'yellow',
                    transition_time: 1551958998
                },
                {
                    in_progress: false,
                    name: 'bamboozled_adblock',
                    outdated: 0,
                    state: 'green',
                    transition_time: 1551959000
                }
            ],
            livejournal: [
                {
                    in_progress: false,
                    name: '5xx_errors',
                    outdated: 1,
                    state: 'green',
                    transition_time: 1551958973
                },
                {
                    in_progress: false,
                    name: 'rps_errors',
                    outdated: 1,
                    state: 'green',
                    transition_time: 1551958987
                }
            ]
        };

        test('should return red state', () => {
            expect(prepareStates(stateItems).autoru.state).toEqual('red');
        });

        test('should return yellow state', () => {
            expect(prepareStates(stateItems).echomsk.state).toEqual('yellow');
        });

        test('should return green state', () => {
            expect(prepareStates(stateItems).livejournal.state).toEqual('green');
        });

        test('should return 0 outdated', () => {
            expect(prepareStates(stateItems).autoru.countOutdated).toEqual(0);
        });

        test('should return 1 outdated', () => {
            expect(prepareStates(stateItems).echomsk.countOutdated).toEqual(1);
        });

        test('should return 2 checks', () => {
            expect(prepareStates(stateItems).livejournal.countAllChecks).toEqual(2);
        });
    });
});
