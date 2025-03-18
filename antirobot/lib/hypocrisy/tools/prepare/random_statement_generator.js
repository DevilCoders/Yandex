const ast = require('shift-ast');

const astUtils = require('./ast_utils');
const utils = require('./utils');

class RandomStatementGenerator {
    constructor(session, literals) {
        this.session = session;
        this.literals = literals;
        this.locals = [];
        this.numDummyLocals = 0;

        this.statementMakers = [
            this._nextVariableDeclaration,
            this._nextFunctionCall
        ].map(f => f.bind(this));
    }

    process(stmt) {
        if (stmt.type === 'VariableDeclarationStatement') {
            for (const declarator of stmt.declaration.declarators) {
                if (declarator.binding.type === 'BindingIdentifier') {
                    this.locals.push(declarator.binding.name);
                }
            }
        }
    }

    next() {
        return utils.randomChoice(this.statementMakers)();
    }

    _nextVariableDeclaration() {
        const expression = this._makeReference();
        const name = this.makeLocalName();

        return new ast.VariableDeclarationStatement({
            declaration: new ast.VariableDeclaration({
                kind: 'var',
                declarators: [new ast.VariableDeclarator({
                    binding: new ast.BindingIdentifier({name}),
                    init: expression
                })]
            })
        });
    }

    _nextFunctionCall() {
        return new ast.ExpressionStatement({expression: this._makeFunctionCall()});
    }

    _makeFunctionCall(options) {
        options = options ?? {};
        options.maxArgs = options.maxArgs ?? 4;
        options.literalArgProbability = options.literalArgProbability ?? 0.4;

        const numArgs = utils.random(0, options.maxArgs);
        let args = [];

        for (let i = 0; i < numArgs; ++i) {
            if (this.locals.length === 0 || Math.random() < options.literalArgProbability) {
                args.push(astUtils.makeDirectLiteralExpression(utils.randomChoice(this.literals)));
            } else {
                args.push(new ast.IdentifierExpression({name: utils.randomChoice(this.locals)}));
            }
        }

        return astUtils.makeRandomDummyCall(args);
    }

    makeLocalName() {
        const name = `pgrdLocal${this.numDummyLocals}`;
        this.locals.push(name);
        ++this.numDummyLocals;

        return name;
    }

    obfuscateExpression(expr, depth) {
        depth = depth ?? (Math.random() < 0.8 ? 2 : 3);

        const exprOffset = utils.random(0, Math.pow(2, depth - 1));
        return this._buildObfuscatedExpression(expr, depth, exprOffset);
    }

    _buildObfuscatedExpression(expr, depth, offset) {
        if (depth === 1) {
            if (offset === 0) {
                return expr;
            }

            return this._makeReference();
        }

        const numLeft = Math.pow(2, depth - 2);
        const left = this._buildObfuscatedExpression(expr, depth - 1, offset);
        const right = this._buildObfuscatedExpression(expr, depth - 1, offset - numLeft);
        const condition = numLeft > offset ? this._makeTrueExpression() : this._makeFalseExpression();

        return new ast.ConditionalExpression({
            test: condition,
            consequent: left,
            alternate: right
        });
    }

    _makeReference() {
        if (this.locals.length === 0 || Math.random() < 0.5) {
            return astUtils.makeDirectLiteralExpression(utils.randomChoice(this.literals));
        } else {
            return new ast.IdentifierExpression({name: utils.randomChoice(this.locals)});
        }
    }

    _makeTrueExpression() {
        return new ast.BinaryExpression({
            left: this._makeFunctionCall(),
            operator: '===',
            right: this._makeFunctionCall()
        });
    }

    _makeFalseExpression() {
        return new ast.BinaryExpression({
            left: this._makeFunctionCall(),
            operator: '<',
            right: this._makeFunctionCall()
        });
    }
}

module.exports = {
    RandomStatementGenerator
};
