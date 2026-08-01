"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.trimCommands = void 0;
function trimCommands(commands, precision = 1) {
    return commands.map(cmd => {
        if (typeof cmd === 'string') {
            return cmd;
        }
        else {
            return {
                x: +cmd.x.toFixed(precision),
                y: +cmd.y.toFixed(precision),
            };
        }
    });
}
exports.trimCommands = trimCommands;
//# sourceMappingURL=trimmer.js.map