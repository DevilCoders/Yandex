import {prepareAlerts} from 'app/lib/alerts-logic';

describe('alerts-logic', () => {
    describe('prepareAlerts', () => {
        const LIMIT = 5;
        const alertsItems = {
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
                    outdated: 1,
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
                    state: 'yellow',
                    transition_time: 1551958973
                },
                {
                    in_progress: false,
                    name: 'rps_errors',
                    outdated: 1,
                    state: 'red',
                    transition_time: 1551958987
                }
            ]
        };

        test('should return object in top with 1 alert if limit >= alerts', () => {
            expect(prepareAlerts(alertsItems, LIMIT).topAlerts.autoru.length).toEqual(1);
        });

        test('should return object in tail with 1 alert if limit < alerts', () => {
            expect(prepareAlerts(alertsItems, 0).tailAlerts.autoru.length).toEqual(1);
        });

        test('should return object in top with queue priority {red, yellow}', () => {
            expect(prepareAlerts(alertsItems, LIMIT).topAlerts.autoru[0].state).toEqual('red');
        });
    });
});
