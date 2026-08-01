"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.generatePaths = void 0;
const paperLoader_1 = require("./paperLoader");
const paper = (0, paperLoader_1.loadPaper)();
function generatePaths(svg) {
    return generatePathsRecursive(svg);
}
exports.generatePaths = generatePaths;
function generatePathsRecursive(item) {
    const paths = [];
    for (const child of item.children) {
        if (child instanceof paper.Group) {
            const innerPaths = generatePathsRecursive(child);
            paths.push(...innerPaths);
        }
        else if (child instanceof paper.Path || child instanceof paper.CompoundPath) {
            paths.push(child);
        }
    }
    return paths;
}
//# sourceMappingURL=generator.js.map