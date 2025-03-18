const CHACHA_QUARTER_ROUNDS = [
    [0, 4, 8, 12],
    [1, 5, 9, 13],
    [2, 6, 10, 14],
    [3, 7, 11, 15],
    [0, 5, 10, 15],
    [1, 6, 11, 12],
    [2, 7, 8, 13],
    [3, 4, 9, 14]
];

function mix(params, numRounds) {
    let output = Array.from(params);

    for (let i = 0; i < numRounds; i += 1) {
        performDoubleRound(output);
    }

    for (let i = 0; i < output.length; i += 1) {
        output[i] = (output[i] + params[i]) >>> 0;
    }

    return output;
}

function performDoubleRound(output) {
    for (const quarterRound of CHACHA_QUARTER_ROUNDS) {
        performQuarterRound(output, ...quarterRound);
    }
}

function performQuarterRound(output, a, b, c, d) {
    output[a] += output[b];
    output[d] = rotl(output[d] ^ output[a], 16);

    output[c] += output[d];
    output[b] = rotl(output[b] ^ output[c], 12);

    output[a] += output[b];
    output[d] = rotl(output[d] ^ output[a], 8);

    output[c] += output[d];
    output[b] = rotl(output[b] ^ output[c], 7);

    output[a] >>>= 0;
    output[b] >>>= 0;
    output[c] >>>= 0;
    output[d] >>>= 0;
}

function rotl(operand, shift) {
    return ((operand << shift) | (operand >>> (32 - shift))) >>> 0;
}

module.exports = {
    CHACHA_QUARTER_ROUNDS,
    mix,
    performDoubleRound,
    performQuarterRound
}
