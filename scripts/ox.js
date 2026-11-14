const readline = require('node:readline');
const { stdin: input, stdout: output } = require('node:process');

const rl = readline.createInterface({ input, output });

rl.on('line', (data) => {
    console.log(
        data.split(' ')
            .map((byte) => `0x${byte}`)
            .join(', ')
    );
});
