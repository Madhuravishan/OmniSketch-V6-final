"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const toSvgJson_1 = require("./toSvgJson");
const toCommands_1 = require("./toCommands");
const vectorizer_1 = require("./vectorizer");
const types_1 = require("./types");
const updateStatusFn = (status) => {
    self.postMessage({
        type: "status",
        payload: status,
    });
};
self.onmessage = async (e) => {
    if (isVectorizeRequest(e.data)) {
        vectorize(e.data);
    }
    else if (isRenderSvgRequest(e.data)) {
        await render(e.data);
    }
    else {
        throw new Error("Bad request");
    }
};
function vectorize(request) {
    updateStatusFn("Vectorizing");
    const svgString = (0, vectorizer_1.vectorizeImageData)(request.raster, request.turdSize);
    self.postMessage({
        type: "vectorizer",
        payload: {
            svg: svgString,
        }
    });
}
async function render(request) {
    const renderResult = await (0, toCommands_1.renderSvgJsonToCommands)(request, updateStatusFn);
    const resultSvgJson = (0, toSvgJson_1.renderCommandsToSvgJson)(renderResult.commands, request.width, request.height, updateStatusFn);
    self.postMessage({
        type: "renderer",
        payload: {
            commands: renderResult.commands,
            svgJson: resultSvgJson,
            distance: renderResult.distance,
            drawDistance: renderResult.drawDistance,
        }
    });
}
function isVectorizeRequest(obj) {
    if (!('type' in obj) || obj.type !== 'vectorize') {
        return false;
    }
    if (!('raster' in obj) || typeof obj.raster !== 'object') {
        return false;
    }
    if (!('turdSize' in obj) || typeof obj.turdSize !== 'number') {
        return false;
    }
    return true;
}
function isRenderSvgRequest(obj) {
    if (!('type' in obj) || obj.type !== 'renderSvg') {
        return false;
    }
    if (!('svgJson' in obj) || typeof obj.svgJson !== 'string') {
        return false;
    }
    if (!('width' in obj) || typeof obj.width !== 'number') {
        return false;
    }
    if (!('height' in obj) || typeof obj.height !== 'number') {
        return false;
    }
    if (!('svgWidth' in obj) || typeof obj.svgWidth !== 'number') {
        return false;
    }
    if (!('svgHeight' in obj) || typeof obj.svgHeight !== 'number') {
        return false;
    }
    if (!('homeX' in obj) || typeof obj.homeX !== 'number') {
        return false;
    }
    if (!('homeY' in obj) || typeof obj.homeY !== 'number') {
        return false;
    }
    if (!('infillDensity' in obj) || typeof obj.infillDensity !== 'number' || !types_1.InfillDensities.includes(obj.infillDensity)) {
        return false;
    }
    if (!('flattenPaths' in obj) || typeof obj.flattenPaths !== 'boolean') {
        return false;
    }
    return true;
}
//# sourceMappingURL=main.js.map