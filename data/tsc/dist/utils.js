"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isPathWhiteOnly = exports.distanceBetweenPointsSquared = exports.distanceBetweenPoints = exports.getLastPoint = void 0;
//import path from 'path';
//import * as fs from 'fs';
const paperLoader_1 = require("./paperLoader");
const paper = (0, paperLoader_1.loadPaper)();
function getLastPoint(commandList) {
    for (let i = commandList.length - 1; i >= 0; i--) {
        const command = commandList[i];
        if (typeof command === 'string') {
            continue;
        }
        else {
            return command;
        }
    }
    return undefined;
}
exports.getLastPoint = getLastPoint;
function distanceBetweenPoints(cmd1, cmd2) {
    return Math.sqrt(Math.pow(cmd2.x - cmd1.x, 2) + Math.pow(cmd2.y - cmd1.y, 2));
}
exports.distanceBetweenPoints = distanceBetweenPoints;
function distanceBetweenPointsSquared(cmd1, cmd2) {
    return Math.pow(cmd2.x - cmd1.x, 2) + Math.pow(cmd2.y - cmd1.y, 2);
}
exports.distanceBetweenPointsSquared = distanceBetweenPointsSquared;
function isPathWhiteOnly(path) {
    return !!(path.fillColor && path.fillColor.toCSS(true) === '#ffffff' && !path.strokeColor);
}
exports.isPathWhiteOnly = isPathWhiteOnly;
// export function dumpSVG(svg: paper.Item) {
//     const svgString = svg.exportSVG({
//         asString: true,
//     }) as string;
//     return dumpStringAsSvg(svgString);
// }
// export async function dumpCanvas(canvas: Canvas) {
//     const fullPath = path.join(__dirname, '../svgs/out.png');
//     fs.writeFileSync(fullPath, canvas.toBuffer());
// }
// export async function dumpStringAsSvg(svgString: string) {
//     const fullPath = path.join(__dirname, '../svgs/out.svg');
//     fs.writeFileSync(fullPath, svgString);
// }
//# sourceMappingURL=utils.js.map