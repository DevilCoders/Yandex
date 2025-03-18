const { asyncTake } = require('iter-tools');
const ast = require('shift-ast');

const chacha = require('./chacha');
const utils = require('./utils');

function* emitInitialization(stateId, key, nonce, counter, numRounds) {
    yield new ast.VariableDeclarationStatement({declaration: new ast.VariableDeclaration({
        kind: 'var',
        declarators: [
            new ast.VariableDeclarator({
                binding: new ast.BindingIdentifier({name: stateId}),
                init: new ast.ArrayExpression({elements: []})
            })
        ]
    })});

    const initState = [
        0x6f626675, 0x73636174, 0x65206772, 0x65656421
    ];

    for (let i = 0; i < 8; ++i) {
        initState.push(utils.readLittle32(key, i * 4));
    }

    initState.push(counter);

    for (let i = 0; i < 3; ++i) {
        initState.push(utils.readLittle32(nonce, i * 4));
    }

    for (let i = 0; i < initState.length; ++i) {
        yield new ast.ExpressionStatement({expression: new ast.AssignmentExpression({
            binding: new ast.ComputedMemberAssignmentTarget({
                object: new ast.IdentifierExpression({name: stateId}),
                expression: new ast.LiteralNumericExpression({value: i})
            }),
            expression: getState(initState[i])
        })});
    }

    const stateExpr = new ast.IdentifierExpression({name: stateId});

    for (let roundsLeft = numRounds; roundsLeft > 0;) {
        const loopLength = roundsLeft > 3 ?
            utils.random(3, Math.min(10, roundsLeft) + 1) :
            roundsLeft;

        yield makeRoundLoop(stateExpr, loopLength);
        roundsLeft -= loopLength;
    }

    for (let i = 0; i < initState.length; ++i) {
        yield new ast.ExpressionStatement({expression: new ast.CompoundAssignmentExpression({
            binding: new ast.ComputedMemberAssignmentTarget({
                object: new ast.IdentifierExpression({name: stateId}),
                expression: new ast.LiteralNumericExpression({value: i})
            }),
            operator: '+=',
            expression: getState(initState[i])
        })});
    }
}

function getState(x) {
    return typeof(x) === 'number' ? new ast.LiteralNumericExpression({value: x}) : x;
}

function makeRoundLoop(output, loopLength) {
    let loopStatements = [];

    for (const quarterRound of chacha.CHACHA_QUARTER_ROUNDS) {
        utils.extend(loopStatements, emitQuarterRoundStatements(output, ...quarterRound));
    }

    return new ast.ForStatement({
        init: new ast.VariableDeclaration({
            kind: "var",
            declarators: [new ast.VariableDeclarator({
                binding: new ast.BindingIdentifier({name: "pgrdIdx"}),
                init: new ast.LiteralNumericExpression({value: 0})
            })]
        }),
        test: new ast.BinaryExpression({
            left: new ast.IdentifierExpression({name: "pgrdIdx"}),
            operator: "<",
            right: new ast.LiteralNumericExpression({value: loopLength})
        }),
        update: new ast.UpdateExpression({
            isPrefix: utils.random(0, 2) == 0,
            operator: "++",
            operand: new ast.AssignmentTargetIdentifier({name: "pgrdIdx"})
        }),
        body: new ast.BlockStatement({block: new ast.Block({statements: loopStatements})})
    });
}

function* emitQuarterRoundStatements(output, a, b, c, d) {
    yield* emitQuarterRoundStepStatements(output, d, a, b, 16);
    yield* emitQuarterRoundStepStatements(output, b, c, d, 12);
    yield* emitQuarterRoundStepStatements(output, d, a, b, 8);
    yield* emitQuarterRoundStepStatements(output, b, c, d, 7);

    for (const x of [a, b, c, d]) {
        yield new ast.ExpressionStatement({expression: new ast.CompoundAssignmentExpression({
            binding: new ast.ComputedMemberAssignmentTarget({
                object: output,
                expression: new ast.LiteralNumericExpression({value: x})
            }),
            operator: ">>>=",
            expression: new ast.LiteralNumericExpression({value: 0})
        })});
    }
}

function* emitQuarterRoundStepStatements(output, x, y, z, shift) {
    yield new ast.ExpressionStatement({
        expression: makeArrayIndexAdditionAssignment(output, y, makeArrayIndexExpression(output, z))
    });

    const rotlExpr = makeRotlExpression(
        new ast.BinaryExpression({
            left: makeArrayIndexExpression(output, x),
            operator: "^",
            right: makeArrayIndexExpression(output, y)
        }),
        shift
    );

    yield new ast.ExpressionStatement({
        expression: makeArrayIndexAssignment(output, x, rotlExpr)
    });
}

function makeArrayIndexAssignment(arr, index, expr) {
    return new ast.AssignmentExpression({
        binding: new ast.ComputedMemberAssignmentTarget({
            object: arr,
            expression: new ast.LiteralNumericExpression({value: index})
        }),
        expression: expr
    });
}

function makeArrayIndexAdditionAssignment(arr, index, expr) {
    return new ast.CompoundAssignmentExpression({
        binding: new ast.ComputedMemberAssignmentTarget({
            object: arr,
            expression: new ast.LiteralNumericExpression({value: index})
        }),
        operator: "+=",
        expression: expr
    });
}

function makeArrayIndexExpression(arr, index) {
    return new ast.ComputedMemberExpression({
        object: arr,
        expression: new ast.LiteralNumericExpression({value: index})
    });
}

function makeRotlExpression(operand, shift) {
    return makeUnsignedRightShift(new ast.BinaryExpression({
        left: new ast.BinaryExpression({
            left: operand,
            operator: '<<',
            right: new ast.LiteralNumericExpression({value: shift})
        }),
        operator: '|',
        right: makeUnsignedRightShift(operand, new ast.BinaryExpression({
            left: new ast.LiteralNumericExpression({value: 32}),
            operator: '-',
            right: new ast.LiteralNumericExpression({value: shift})
        }))
    }), 0);
}

function makeUnsignedRightShift(operand, shift) {
    if (typeof(shift) === 'number') {
        shift = new ast.LiteralNumericExpression({value: shift});
    }

    return new ast.BinaryExpression({
        left: operand,
        operator: '>>>',
        right: shift
    });
}

module.exports = {
    emitInitialization,
    makeRoundLoop
};
