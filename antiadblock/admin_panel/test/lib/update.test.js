import update from 'app/lib/update';

describe('update', () => {
    describe('$byPath', () => {
        test('simple path', () => {
            expect(update(
                {first: {third: 'third', fourth: 'fourth'}, second: 'second'},
                {
                    $byPath: {
                        path: 'first.third',
                        do: {
                            $set: 'new_value'
                        }
                    }
                }
            )).toEqual({first: {third: 'new_value', fourth: 'fourth'}, second: 'second'});
        });

        test('path with number', () => {
            expect(update(
                {first: {2: {name: 'name'}}},
                {
                    $byPath: {
                        path: 'first.2.name',
                        do: {
                            $set: 'new_name'
                        }
                    }
                }
            )).toEqual({first: {2: {name: 'new_name'}}});
        });
    });

    describe('$unsetByPath', () => {
        test('unset by simple key', () => {
            expect(update(
                {first: 'first', second: 'second'},
                {
                    $unsetByPath: 'first'
                }
            )).toEqual({second: 'second'});
        });

        test('unset by path', () => {
            expect(update(
                {first: {third: 'third', fourth: 'fourth'}, second: 'second'},
                {
                    $unsetByPath: 'first.third'
                }
            )).toEqual({first: {fourth: 'fourth'}, second: 'second'});
        });

        test('unset by multiple paths', () => {
            expect(update(
                {first: {third: 'third', fourth: 'fourth'}, second: 'second'},
                {
                    $unsetByPath: ['first.third', 'second']
                }
            )).toEqual({first: {fourth: 'fourth'}});
        });

        test('unset must be immutable', () => {
            const original = {first: 'first', second: 'second', third: {fourth: 'fourth'}};

            update(original, {$unsetByPath: 'first'});

            expect(original).toEqual({first: 'first', second: 'second', third: {fourth: 'fourth'}});
        });
    });

    describe('$initArray', () => {
        test('base', () => {
            expect(update(
                {first: null, second: true},
                {
                    first: {
                        $initArray: {}
                    }
                }
            )).toEqual({first: [], second: true});
        });
    });

    describe('$initAndPush', () => {
        test('push array', () => {
            expect(update(
                {first: null, second: true},
                {
                    first: {
                        $initAndPush: ['first_value', 'second_value']
                    }
                }
            )).toEqual({first: ['first_value', 'second_value'], second: true});
        });

        test('push string', () => {
            expect(update(
                {first: null, second: true},
                {
                    first: {
                        $initAndPush: 'first_value'
                    }
                }
            )).toEqual({first: ['first_value'], second: true});
        });

        test('push to exists array', () => {
            expect(update(
                {first: ['first_value'], second: true},
                {
                    first: {
                        $initAndPush: 'second_value'
                    }
                }
            )).toEqual({first: ['first_value', 'second_value'], second: true});
        });
    });

    describe('$toggle', () => {
        test('base', () => {
            expect(update(
                {first: true},
                {
                    first: {$toggle: true}
                }
            )).toEqual({first: false});
        });
    });

    describe('$byId', () => {
        test('base', () => {
            expect(update(
                {first: {
                    third: [
                        {_id: 'id_first', value: 'value_first'},
                        {_id: 'id_second', value: 'value_second'}
                    ]
                }, second: 'second'},
                {
                    $byId: {
                        path: 'first.third',
                        key: '_id',
                        id: 'id_second',
                        do: {
                            value: {
                                $set: 'new_value'
                            }
                        }
                    }
                }
            )).toEqual({first: {
                third: [
                    {_id: 'id_first', value: 'value_first'},
                    {_id: 'id_second', value: 'new_value'}
                ]
            }, second: 'second'});
        });

        test('fallback', () => {
            expect(update(
                {first: {
                    third: [
                        {_id: 'id_first', value: 'value_first'},
                        {_id: 'id_second', value: 'value_second'}
                    ]
                }, second: 'second'},
                {
                    $byId: {
                        path: 'first.third',
                        key: '_id',
                        id: 'id_third',
                        do: {
                            value: {
                                $set: 'new_value'
                            }
                        },
                        fallback: {
                            $initAndPush: {
                                _id: 'id_third',
                                value: 'value_third'
                            }
                        }
                    }
                }
            )).toEqual({first: {
                third: [
                    {_id: 'id_first', value: 'value_first'},
                    {_id: 'id_second', value: 'value_second'},
                    {_id: 'id_third', value: 'value_third'}
                ]
            }, second: 'second'});
        });
    });

    describe('$unsetByValue', () => {
        test('unset by simple value', () => {
            expect(update(
                ['first', 'second'],
                {
                    $unsetByValue: 'first'
                }
            )).toEqual(['second']);
        });

        test('unset by object value', () => {
            expect(update(
                [{first: 'first'}, 'second', null, false],
                {
                    $unsetByValue: [{first: 'first'}, null]
                }
            )).toEqual(['second', false]);
        });

        test('unset by multiple keys', () => {
            expect(update(
                ['first', 'second', 'third'],
                {
                    $unsetByValue: ['first', 'third']
                }
            )).toEqual(['second']);
        });

        test('unset must be immutable', () => {
            const original = ['first', 'second', {third: 'third'}];

            update(original, {$unsetByValue: 'first'});

            expect(original).toEqual(['first', 'second', {third: 'third'}]);
        });
    });
});
