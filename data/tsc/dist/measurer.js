"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.measureDistance = void 0;
const utils_1 = require("./utils");
function measureDistance(dedupedCommands) {
    let totalDistance = 0;
    let drawDistance = 0;
    let penUp = true;
    for (let i = 1; i < dedupedCommands.length; i++) {
        const command = dedupedCommands[i];
        if (typeof command !== 'string') {
            const lastCommand = (0, utils_1.getLastPoint)(dedupedCommands.slice(0, i));
            if (lastCommand) {
                if (command.x !== lastCommand.x || command.y !== lastCommand.y) {
                    const distance = (0, utils_1.distanceBetweenPoints)(lastCommand, command);
                    totalDistance += distance;
                    if (!penUp) {
                        drawDistance += distance;
                    }
                }
            }
        }
        else {
            if (command === 'p0') {
                penUp = true;
            }
            else if (command === 'p1') {
                penUp = false;
            }
        }
    }
    return {
        totalDistance,
        drawDistance,
    };
}
exports.measureDistance = measureDistance;
//# sourceMappingURL=measurer.js.map