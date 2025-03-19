const {generateApi} = require('swagger-typescript-api');
const {resolve} = require('path');
const fs = require('fs');

generateApi({
    name: "deploy.ts",
    input: resolve(process.cwd(), '../deploy/api/api/swagger.yaml'),
    generateResponses: true,
})
    .then(sourceFile => fs.writeFile(
        resolve(process.cwd(), './src/models/deployapi.ts'),
        sourceFile,
        e => console.info(e || 'Write success!')))
    .catch(e => console.error(e));
