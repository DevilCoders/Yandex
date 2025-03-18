const ast = require('shift-ast');
const { parseScript } = require('shift-parser');

const utils = require('./utils');

const NUM_CALLABLE_DUMMIES = 8;

function convertLiteralToAstNode(literal) {
    if (typeof(literal) === 'string') {
        return new ast.LiteralStringExpression({value: literal});
    } else if (typeof(literal) === 'number') {
        return new ast.LiteralNumericExpression({value: literal});
    } else {
        throw new Error('unexpected literal type: ' + typeof(literal));
    }
}

function makeGetLiteralFunction() {
    const getLiteralFunctionSource = `
        function pgrdLit(index) {
            return pgrdLiterals[index];
        }`;

    return parseScript(getLiteralFunctionSource).statements[0];
}

function makeLiteralExpression(literalToIndex, literal) {
    const index = literalToIndex.get(literal);

    if (index === undefined) {
        throw new Error(`unexpected literal: ${literal} (${typeof(literal)})`);
    }

    return new ast.CallExpression({
        callee: new ast.IdentifierExpression({name: 'pgrdLit'}),
        arguments: [new ast.LiteralNumericExpression({value: index})]
    });
}

function makeDirectLiteralExpression(literal) {
    if (typeof(literal) === 'string') {
        return new ast.LiteralStringExpression({value: literal});
    } else if (typeof(literal) === 'number') {
        return new ast.LiteralNumericExpression({value: literal});
    } else {
        throw new Error('unexpected literal type: ' + typeof(literal));
    }
}

function makeCallableDummy(i) {
    const callableDummySource = `function ${makeCallableDummyName(i)}() {}`;
    return parseScript(callableDummySource).statements[0];
}

function makeCallableDummyName(i) {
    return `pgrdCallableDummy${i}`;
}

function makeRandomDummyCall(args) {
    return new ast.CallExpression({
        callee: new ast.StaticMemberExpression({
            object: new ast.IdentifierExpression({name: 'window'}),
            property: makeCallableDummyName(utils.random(0, NUM_CALLABLE_DUMMIES))
        }),
        arguments: args
    });
}

function isNumericOrStringLiteral(type) {
    return type === 'LiteralNumericExpression' || type === 'LiteralStringExpression';
}

function checkNodeType(description, node, expectedType) {
    if (node.type !== expectedType) {
        throw new Error(`expected ${description} type ${expectedType} but got ${node.type}`);
    }
}

module.exports = {
    NUM_CALLABLE_DUMMIES,
    convertLiteralToAstNode,
    makeGetLiteralFunction,
    makeLiteralExpression,
    makeDirectLiteralExpression,
    makeCallableDummy,
    makeCallableDummyName,
    makeRandomDummyCall,
    isNumericOrStringLiteral,
    checkNodeType,
};
