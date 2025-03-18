module.exports = {
    env: {
        browser: true,
        commonjs: true,
        es6: true,
        node: true,
        jest: true
    },
    extends: [
        'xo',
        'xo-react'
    ],
    parserOptions: {
        ecmaFeatures: {
            experimentalObjectRestSpread: true,
            jsx: true
        },
        ecmaVersion: 6,
        sourceType: 'module'
    },
    plugins: [
        'react'
    ],
    rules: {
        'space-before-function-paren': ['off'],
        indent: 'off',
        quotes: ['error', 'single', {avoidEscape: true}],
        'capitalized-comments': 0,
        'react/jsx-indent': ['error', 4],
        'react/jsx-indent-props': ['error', 4],
        'react/jsx-sort-props': 0,
        'react/jsx-closing-bracket-location': ['error', 'after-props'],
        'react/jsx-tag-spacing': ['error', {
            beforeSelfClosing: 'always'
        }],
        'jsx-quotes': ['error', 'prefer-single'],
        'react/jsx-handler-names': ['off'],
        'react/require-default-props': 0,
        'react/no-danger': 0,
        'no-console': ['error'],
        'one-var': ['error', {
            'var': 'always',
            'let': 'always',
            'const': 'never'
        }],
        'no-mixed-requires': 'off',
        'no-negated-condition': 'off',
        'camelcase': ['error', {
            'properties': 'never'
        }],
        'padding-line-between-statements': [
            'error',
            { blankLine: 'always', prev: ['let', 'var'], next: '*'},
            { blankLine: 'any',    prev: ['let', 'var'], next: ['const', 'let', 'var']}
        ],
        'function-paren-newline': 'off',
        'no-else-return': ['error', {
               allowElseIf: true
        }]
    }
};
