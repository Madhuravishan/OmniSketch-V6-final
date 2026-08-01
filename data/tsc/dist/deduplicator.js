"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.dedupeCommands = void 0;
const utils_1 = require("./utils");
function dedupeCommands(commands) {
    const dedupedCommands = [];
    for (const command of commands) {
        if (typeof command === 'string') {
            if (dedupedCommands.length === 0 || dedupedCommands[dedupedCommands.length - 1] !== command) {
                dedupedCommands.push(command);
            }
        }
        else {
            const lastCommand = (0, utils_1.getLastPoint)(dedupedCommands);
            if (lastCommand) {
                if (command.x !== lastCommand.x || command.y !== lastCommand.y) {
                    dedupedCommands.push(command);
                }
            }
            else {
                dedupedCommands.push(command);
            }
        }
    }
    const filteredCommands = [];
    for (let i = 0; i < dedupedCommands.length; i++) {
        if (i == 0) {
            filteredCommands.push(dedupedCommands[0]);
        }
        else {
            const currentCommand = dedupedCommands[i];
            const previousCommand = filteredCommands[filteredCommands.length - 1];
            if (previousCommand === 'p0' && currentCommand === 'p1') {
                //verify that we were p1 before that last p0
                for (let i = filteredCommands.length - 2; i >= 0; i--) {
                    const command = filteredCommands[i];
                    if (typeof command === 'string' && command.charAt(0) === 'p') {
                        if (command.charAt(1) === '1') {
                            // all is well
                            break;
                        }
                        else {
                            throw new Error('Inconsistent pen movement');
                        }
                    }
                }
                filteredCommands.pop();
            }
            else {
                filteredCommands.push(currentCommand);
            }
        }
    }
    return filteredCommands;
}
exports.dedupeCommands = dedupeCommands;
//# sourceMappingURL=deduplicator.js.map