const { Buffer } = require('buffer');
const crypto = require('crypto');
const fs = require('fs');

const babel = require('@babel/core');
const iter = require('iter-tools');
const { obfuscate: contribObfuscate } = require('javascript-obfuscator');
const terser = require('terser');
const yargs = require('yargs/yargs');

const ast = require('shift-ast');
const { parseScript } = require('shift-parser');
const { refactor, isShiftNode } = require('shift-refactor');
const queryAst = require('shift-query');

const astUtils = require('./ast_utils');
const chachaAst = require('./chacha_ast');
const { RandomStatementGenerator } = require('./random_statement_generator');
const utils = require('./utils');

async function main() {
    const args = yargs(process.argv.slice(2))
        .command('$0', 'prepare_greed', cmd => { cmd.strict(); })
        .usage('Usage: prepare_greed --greed path/to/greed.js [options]')
        .strict()
        .option('greed', {
            type: 'string',
            demand: true,
            description: 'Path to compiled Greed js file'
        })
        .option('rename-greed', {
            type: 'string',
            default: null,
            description: 'Rename Greed to this name'
        })
        .option('disable-crypto', {
            type: 'boolean',
            default: false,
            description: 'Disable crypto for debugging'
        })
        .option('descriptor-key', {
            type: 'string',
            demand: true,
            description: 'Path to descriptor encryption key file'
        })
        .option('generation-time', {
            type: 'number',
            default: Math.floor(new Date().getTime() / 1000 + 24 * 60 * 60),
            description: 'Generation timestamp in seconds'
        })
        .option('output-script', {
            type: 'string',
            demand: true,
            description: 'Path to output script file'
        })
        .option('growth-factor', {
            type: 'number',
            default: 1.1,
            description: 'Expected growth of script'
        })
        .option('obfuscate', {
            type: 'boolean',
            default: false,
            description: 'Run our custom obfuscator'
        })
        .option('contrib-obfuscate', {
            type: 'boolean',
            default: false,
            description: 'Run obfuscator.io'
        })
        .option('contrib-obfuscate-override', {
            type: 'string',
            default: '{}',
            description: 'Override default obfuscator.io options (in JSON)'
        })
        .option('terser', {
            type: 'boolean',
            default: false,
            description: 'Run terser'
        })
        .option('babel', {
            type: 'boolean',
            default: false,
            description: 'Run babel'
        })
        .argv;

    const greedFileContent = fs.readFileSync(args.greed, 'utf8');
    const descriptorKey = Buffer.from(fs.readFileSync(args.descriptorKey, 'utf8'), 'base64');
    let output = prepareGreed(
        greedFileContent,
        args.renameGreed,
        args.disableCrypto,
        descriptorKey,
        args.generationTime,
        args.growthFactor,
        args.obfuscate
    );

    if (args.contribObfuscate) {
        const defaultOptions = {
            controlFlowFlattening: true,
            controlFlowFlatteningThreshold: 0.5,
            deadCodeInjection: true,
            deadCodeInjectionThreshold: 0.3,
            debugProtection: false,
            debugProtectionInterval: false,
            disableConsoleOutput: false
        };

        const overridenOptions = JSON.parse(args.contribObfuscateOverride);

        output = contribObfuscate(output, Object.assign(defaultOptions, overridenOptions))
            .getObfuscatedCode();
    }

    if (args.babel) {
        output = babel.transformSync(output, {presets: ["@babel/preset-env"]}).code;
    }

    if (args.terser) {
        const options = {
            compress: {
                defaults: false
            }
        };

        output = (await terser.minify(output, options)).code;
    }

    fs.writeFileSync(args.outputScript, output, 'utf8');
}

function prepareGreed(
    input,
    newGreedName,
    disableCrypto,
    descriptorKey,
    generationTime,
    growthFactor,
    shouldObfuscate
) {
    if (growthFactor < 1) {
        throw new Error('growth factor cannot be below 1');
    }

    const script = parseScript(input);

    if (newGreedName !== null) {
        renameGreed(script, newGreedName);
    }

    const refactorQuery = refactor(script);

    if (!disableCrypto) {
        const key = crypto.randomBytes(32);
        const nonce =  crypto.randomBytes(12);
        const numRounds = utils.random(10, 20);
        injectCrypto(refactorQuery, descriptorKey, generationTime, key, nonce, numRounds);
    }

    if (shouldObfuscate) {
        obfuscate(refactorQuery, script, growthFactor);
    }

    return refactorQuery.codegen()[0];
}

function renameGreed(script, newName) {
    utils.visit(script, property => {
        if (property == null) {
            return {proceed: false};
        } else if (property === 'Greed') {
            return {proceed: false, substitute: newName};
        } else if (property.type == null) {
            return {proceed: property instanceof Array};
        }

        return {proceed: true};
    });
}

function injectCrypto(refactorQuery, descriptorKey, generationTime, key, nonce, numRounds) {
    const {prologue, assignments, epilogue} = findMarkers(refactorQuery);

    const counterDecl = parseScript('var pgrdCryptoCounter = 0;').statements[0];
    prologue.replace(node => counterDecl);

    const encryptionStmts = parseScript(
        'function fun() {' +
        '    pgrdPlaintext = pgrdPlaintext == null ? "null" : pgrdPlaintext.toString();' +
        '    var pgrdCiphertext = [];' +
        '    for (var pgrdIdx = 0; pgrdIdx < pgrdPlaintext.length; ++pgrdIdx) {' +
        '        var c = (pgrdCryptoState[Math.floor(pgrdIdx / 2) % pgrdCryptoState.length] >>> (16 * (pgrdIdx % 2))) & 0xFFFF;' +
        '        pgrdCiphertext.push(String.fromCharCode(' +
        '            pgrdPlaintext.charCodeAt(pgrdIdx) & 0xFF ^' +
        '            (c & 0xFF)' +
        '        ));' +
        '        pgrdCiphertext.push(String.fromCharCode(' +
        '            (pgrdPlaintext.charCodeAt(pgrdIdx) >>> 8) ^' +
        '            (c >>> 8)' +
        '        ));' +
        '    }' +
        '    var pgrdRet = btoa(pgrdCiphertext.join("")) + ";" + pgrdCryptoCounter.toString();' +
        '    ++pgrdCryptoCounter;' +
        '    return pgrdRet;' +
        '}'
    )
        .statements[0].body.statements;

    assignments.replace(
        node => makeValueCrypto(encryptionStmts, key, nonce, numRounds, node.arguments[0]),
    );

    const typeAssignmentStatement = new ast.ExpressionStatement({
        expression: new ast.AssignmentExpression({
            binding: new ast.AssignmentTargetIdentifier({
                name: epilogue.raw().arguments[0].name,
            }),
            expression: astUtils.makeDirectLiteralExpression('object'),
        }),
    });

    const epilogueNode = epilogue.raw();
    let appendPoint = epilogue.closest(':statement').append(typeAssignmentStatement);

    const descriptor = makeCryptoDescriptor(descriptorKey, generationTime, key, nonce, numRounds);

    const descriptorAssignmentStatement = new ast.ExpressionStatement({
        expression: new ast.AssignmentExpression({
            binding: new ast.StaticMemberAssignmentTarget({
                object: epilogueNode.arguments[1],
                property: 'pgrd',
            }),
            expression: astUtils.makeDirectLiteralExpression(descriptor),
        }),
    });

    appendPoint = appendPoint.append(descriptorAssignmentStatement);

    const timestampExpression = makeValueCrypto(
        encryptionStmts, key, nonce, numRounds,
        parseScript('Math.floor(new Date().getTime() / 1000)').statements[0].expression,
    );

    const timestampAssignmentStatement = new ast.ExpressionStatement({
        expression: new ast.AssignmentExpression({
            binding: new ast.StaticMemberAssignmentTarget({
                object: epilogueNode.arguments[1],
                property: 'pgrdt',
            }),
            expression: timestampExpression,
        }),
    });

    appendPoint.append(timestampAssignmentStatement);
}

function makeValueCrypto(encryptionStmts, key, nonce, numRounds, expression) {
    const statements = Array.from(iter.concat(
        chachaAst.emitInitialization(
            "pgrdCryptoState",
            key,
            nonce,
            new ast.IdentifierExpression({name: "pgrdCryptoCounter"}),
            numRounds
        ),
        utils.deepCopy(encryptionStmts)
    ));

    return new ast.CallExpression({
        callee: new ast.FunctionExpression({
            isAsync: false,
            isGenerator: false,
            name: null,
            params: new ast.FormalParameters({
                items: [new ast.BindingIdentifier({name: "pgrdPlaintext"})],
                rest: null
            }),
            body: new ast.FunctionBody({
                directives: [],
                statements
            })
        }),
        arguments: [expression]
    });
}

function obfuscate(refactorQuery, script, growthFactor) {
    let literals = Array.from(new Set(findLiterals(refactorQuery)));
    utils.extend(
        literals,
        iter.map(astUtils.makeCallableDummyName, iter.range(astUtils.NUM_CALLABLE_DUMMIES))
    );
    utils.shuffle(literals);
    const literalToIndex = new Map(iter.map((literal, index) => [literal, index], literals));

    // TODO: Improve random statements?

    for (let body of iter.concat(
        refactorQuery('FunctionBody').session.nodes,
        refactorQuery('Block').session.nodes,
    )) {
        body.statements = insertRandomStatements(
            refactorQuery, literals,
            body.statements,
            Math.ceil((growthFactor - 1) * body.statements.length)
        );
    }

    // TODO: Get rid of these? js-obfuscate has similar functionality.

    refactorQuery('LiteralStringExpression, LiteralNumericExpression')
        .replace(node => astUtils.makeLiteralExpression(literalToIndex, node.value));

    refactorQuery('AssignmentExpression[binding.type="StaticMemberAssignmentTarget"]')
        .replace(node => new ast.AssignmentExpression({
            binding: new ast.ComputedMemberAssignmentTarget({
                object: node.binding.object,
                expression: astUtils.makeLiteralExpression(literalToIndex, node.binding.property),
            }),
            expression: node.expression,
        }));

    refactorQuery('StaticMemberExpression')
        .replace(node => new ast.ComputedMemberExpression({
            object: node.object,
            expression: astUtils.makeLiteralExpression(literalToIndex, node.property),
        }));

    refactorQuery('StaticPropertyName')
        .replace(node => new ast.ComputedPropertyName({
            expression: astUtils.makeLiteralExpression(literalToIndex, node.value),
        }));

    let prefix = [
        makeLiteralArray(literals),
        astUtils.makeGetLiteralFunction()
    ];

    for (let i = 0; i < astUtils.NUM_CALLABLE_DUMMIES; ++i) {
        prefix.push(astUtils.makeCallableDummy(i));
    }

    refactorQuery.session.nodes[0].statements.unshift(...prefix);
}

function findMarkers(refactorQuery) {
    return {
        prologue: findPrologue(refactorQuery),
        assignments: findAssignments(refactorQuery),
        epilogue: findEpilogue(refactorQuery),
    };
}

function findPrologue(refactorQuery) {
    const prologue = refactorQuery(
        'CallExpression[callee.type="IdentifierExpression"][callee.name="hypocrisyMarkerPrologue"]',
    );
    const prologueNode = utils.checkNumberOfItems(
        'hypocrisyMarkerPrologue call',
        prologue.session.nodes, 1,
    );

    if (prologueNode.arguments.length != 1) {
        throw new Error('wrong number of arguments passed to hypocrisyMarkerPrologue');
    }

    const prologueArgument = utils.checkNumberOfItems(
        'argument in hypocrisyMarkerPrologue call',
        prologueNode.arguments, 1,
    );

    astUtils.checkNodeType(
        'hypocrisyMarkerPrologue argument',
        prologueArgument,
        'LiteralStringExpression',
    );

    if (prologueArgument.value !== '1') {
        throw new Error('unsupported version of hypocrisy markers');
    }

    return prologue;
}

function findAssignments(refactorQuery) {
    const assignments = refactorQuery(
        'CallExpression[callee.type="IdentifierExpression"][callee.name="hypocrisyMarkerAssignment"]',
    );

    utils.checkNumberOfItems(
        'hypocrisyMarkerAssignment calls',
        assignments.session.nodes, 2,
    );

    return assignments;
}

function findEpilogue(refactorQuery) {
    const epilogue = refactorQuery(
        'CallExpression[callee.type="IdentifierExpression"][callee.name="hypocrisyMarkerEpilogue"]',
    );

    const epilogueNode = utils.checkNumberOfItems(
        'hypocrisyMarkerEpilogue calls',
        epilogue.session.nodes, 1,
    );

    utils.checkNumberOfItems(
        'arguments in hypocrisyMarkerEpilogue',
        epilogueNode.arguments, 2,
    );

    astUtils.checkNodeType(
        'hypocrisyMarkerEpilogue first argument',
        epilogueNode.arguments[0],
        'IdentifierExpression',
    );

    astUtils.checkNodeType(
        'hypocrisyMarkerEpilogue second argument',
        epilogueNode.arguments[1],
        'IdentifierExpression',
    );

    return epilogue;
}

function makeCryptoDescriptor(descriptorKey, generationTime, key, nonce, numRounds) {
    const descriptor = {
        version: 1,
        generationTime,
        key: Buffer.from(key).toString('base64'),
        nonce: Buffer.from(nonce).toString('base64'),
        numRounds: numRounds
    };

    const encodedDescriptor = Buffer.from(JSON.stringify(descriptor), 'utf8');

    const descriptorNonce = crypto.randomBytes(12);
    const cipher = crypto.createCipheriv('aes-256-gcm', descriptorKey, descriptorNonce);
    const encryptedDescriptor = Buffer.concat([
        descriptorNonce,
        cipher.update(encodedDescriptor),
        cipher.final(),
        cipher.getAuthTag()
    ]);

    return encryptedDescriptor.toString('base64');
}

function findLiterals(refactorQuery) {
    const selectors = [
        'LiteralStringExpression',
        'LiteralNumericExpression',
        'AssignmentExpression[binding.type="StaticMemberAssignmentTarget"]',
        'ExpressionStatement[expression.expression.type="StaticMemberExpression"]',
        'StaticMemberExpression',
        'StaticPropertyName',
    ];

    return refactorQuery(selectors.join(', '))
        .map(node => {
            switch (node.type) {
            case 'LiteralStringExpression':
            case 'LiteralNumericExpression':
                return node.value;
            case 'AssignmentExpression':
                return node.binding.property;
            case 'ExpressionStatement':
                return node.expression.expression.property;
            case 'StaticMemberExpression':
                return node.property;
            case 'StaticPropertyName':
                return node.value;
            }
        });
}

function makeLiteralArray(literals) {
    return new ast.VariableDeclarationStatement({
        declaration: new ast.VariableDeclaration({
            kind: 'const',
            declarators: [new ast.VariableDeclarator({
                binding: new ast.BindingIdentifier({name: 'pgrdLiterals'}),
                init: new ast.ArrayExpression({
                    elements: literals.map(astUtils.convertLiteralToAstNode)
                })
            })]
        })
    });
}

function insertRandomStatements(session, literals, statements, numNewStatements) {
    let statementGenerator = new RandomStatementGenerator(session, literals);
    let newStatements = [];
    let numLeftNewStatements = numNewStatements;

    const hasTrailingReturnStmt =
        statements.length > 0 &&
        utils.last(statements).type === 'ReturnStatement';

    const numOldStatements = hasTrailingReturnStmt ? statements.length - 1 : statements.length;

    for (
        let i = 0;
        newStatements.length < numOldStatements + numNewStatements && i < numOldStatements;
    ) {
        if (Math.random() < numLeftNewStatements / (numOldStatements + numLeftNewStatements - i)) {
            --numLeftNewStatements;
            newStatements.push(statementGenerator.next());
        } else {
            statementGenerator.process(statements[i]);
            newStatements.push(statements[i]);
            ++i;
        }
    }

    let numTrailingRandomStatements = numOldStatements + numNewStatements - newStatements.length;

    while (newStatements.length < numOldStatements + numNewStatements) {
        newStatements.push(statementGenerator.next());
    }

    if (hasTrailingReturnStmt) {
        const oldRetExpr = utils.last(statements).expression;

        if (numTrailingRandomStatements > 0 && Math.random() < 0.3) {
            const retExprName = statementGenerator.makeLocalName();
            const retExprDecl = new ast.VariableDeclarationStatement({
                declaration: new ast.VariableDeclaration({
                    kind: 'var',
                    declarators: [new ast.VariableDeclarator({
                        binding: new ast.BindingIdentifier({name: retExprName}),
                        init: oldRetExpr
                    })]
                })
            });

            const spilledRetExprIndex = newStatements.length - utils.random(0, numTrailingRandomStatements);
            newStatements.splice(spilledRetExprIndex, 0, retExprDecl);

            newStatements.push(new ast.ReturnStatement({
                expression: new ast.IdentifierExpression({name: retExprName})
            }));
        } else {
            newStatements.push(new ast.ReturnStatement({expression: oldRetExpr}));
        }
    }

    for (let stmt of newStatements) {
        utils.visit(stmt, property => {
            if (property == null) {
                return {proceed: false};
            } else if (property.type == null) {
                return {proceed: property instanceof Array};
            } else if (property.type.endsWith('Expression')) {
                return {
                    proceed: false,
                    substitute: statementGenerator.obfuscateExpression(property)
                };
            } else {
                return {proceed: !property.type.endsWith('Statement')};
            }
        });
    }

    return newStatements;
}

if (require.main === module) {
    main();
}
