"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.renderPathsToCommands = void 0;
const paperLoader_1 = require("./paperLoader");
const paper = (0, paperLoader_1.loadPaper)();
function renderPathsToCommands(paths, width, height) {
    const viewRectangle = new paper.Rectangle(0, 0, width, height);
    return paths.flatMap(p => {
        if (p.segments.length < 2) {
            return [];
        }
        const commands = ['p0'];
        let started = false;
        let firstSegment = null;
        for (const segment of p.segments) {
            if (viewRectangle.contains(segment.point)) {
                commands.push({
                    x: segment.point.x,
                    y: segment.point.y,
                });
                if (!started) {
                    firstSegment = segment;
                    commands.push('p1');
                    started = true;
                }
            }
        }
        if (firstSegment && p.closed) {
            commands.push({
                x: firstSegment.point.x,
                y: firstSegment.point.y,
            });
        }
        return commands;
    });
}
exports.renderPathsToCommands = renderPathsToCommands;
//# sourceMappingURL=renderer.js.map