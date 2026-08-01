"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.renderSvgJsonToCommands = void 0;
const generator_1 = require("./generator");
const infill_1 = require("./infill");
const optimizer_1 = require("./optimizer");
const renderer_1 = require("./renderer");
const trimmer_1 = require("./trimmer");
const deduplicator_1 = require("./deduplicator");
const measurer_1 = require("./measurer");
const paperLoader_1 = require("./paperLoader");
const flattener_1 = require("./flattener");
const paper = (0, paperLoader_1.loadPaper)();
async function renderSvgJsonToCommands(request, updateStatusFn) {
    paper.setup({ width: request.width, height: request.height });
    updateStatusFn("Importing");
    const svg = paper.project.importJSON(request.svgJson);
    // scale the document so its coordinates match the world 1:1, in mm
    const projectToViewRatio = request.width / request.svgWidth;
    console.log(`Scaling by ${projectToViewRatio}`);
    svg.scale(projectToViewRatio, { x: 0, y: 0 });
    svg.applyMatrix = true;
    updateStatusFn("Generating paths");
    const paths = (0, generator_1.generatePaths)(svg);
    paths.forEach(p => p.flatten(0.5));
    if (request.flattenPaths) {
        (0, flattener_1.flattenPaths)(paths, updateStatusFn);
    }
    updateStatusFn("Generating infill");
    const pathsWithInfills = (0, infill_1.generateInfills)(paths, request.infillDensity);
    updateStatusFn("Optimizing paths");
    const optimizedPaths = (0, optimizer_1.optimizePaths)(pathsWithInfills, request.homeX, request.homeY);
    updateStatusFn("Generating commands");
    const commands = (0, renderer_1.renderPathsToCommands)(optimizedPaths, request.width, request.height);
    commands.push('p0');
    const trimmedCommands = (0, trimmer_1.trimCommands)(commands);
    updateStatusFn("Simplifying commands");
    const dedupedCommands = (0, deduplicator_1.dedupeCommands)(trimmedCommands);
    updateStatusFn("Measuring total distance");
    dedupedCommands.unshift(`h${request.height}`);
    const distances = (0, measurer_1.measureDistance)(dedupedCommands);
    const totalDistance = +distances.totalDistance.toFixed(1);
    dedupedCommands.unshift(`d${totalDistance}`);
    const commandStrings = dedupedCommands.map(stringifyCommand);
    return {
        commands: commandStrings,
        distance: totalDistance,
        drawDistance: +distances.drawDistance.toFixed(1),
    };
}
exports.renderSvgJsonToCommands = renderSvgJsonToCommands;
function stringifyCommand(cmd) {
    if (typeof cmd === 'string') {
        return cmd;
    }
    else {
        return `${cmd.x} ${cmd.y}`;
    }
}
//# sourceMappingURL=toCommands.js.map