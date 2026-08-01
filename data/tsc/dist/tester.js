"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const toSvgJson_1 = require("./toSvgJson");
const vectorizer_1 = require("./vectorizer");
const toCommands_1 = require("./toCommands");
const path_1 = __importDefault(require("path"));
const fs = __importStar(require("fs"));
const canvas_1 = require("canvas");
const paperLoader_1 = require("./paperLoader");
const paper = (0, paperLoader_1.loadPaper)();
const width = 1000;
const renderScaleFactor = 2;
function updater(status) {
    console.log(status);
}
async function main_vectorRasterVector() {
    const dirPath = path_1.default.join(__dirname, '../svgs');
    const inDir = fs.opendirSync(dirPath);
    const outDirPath = path_1.default.join(__dirname, '../svgs/out/');
    let dirEntry = inDir.readSync();
    while (dirEntry) {
        if (dirEntry.isFile() && dirEntry.name.endsWith(".svg")) {
            if (dirEntry.name == "finitecurve.svg") {
                console.log(`processing ${dirEntry.name}`);
                const file = fs.readFileSync(path_1.default.join(dirEntry.path, dirEntry.name));
                const svgString = file.toString();
                const [imageData, svgWidth, svgHeight] = await getImageData(svgString, renderScaleFactor);
                const vectorizedSvg = (0, vectorizer_1.vectorizeImageData)(imageData, 2);
                const vectorizedJson = convertSvgToSvgJson(vectorizedSvg);
                const height = Math.floor(svgHeight * (width / svgWidth));
                const request = {
                    svgJson: vectorizedJson,
                    height,
                    width,
                    svgWidth: width * renderScaleFactor,
                    svgHeight: height * renderScaleFactor,
                    homeX: 0,
                    homeY: 0,
                    infillDensity: 4,
                    type: 'renderSvg',
                    flattenPaths: false,
                };
                const result = await (0, toCommands_1.renderSvgJsonToCommands)(request, updater);
                const resultSvgJsonString = (0, toSvgJson_1.renderCommandsToSvgJson)(result.commands, width, height, updater);
                const resultSvg = convertSvgJsonToSvg(resultSvgJsonString, width, height);
                const fullResultPath = path_1.default.join(outDirPath, dirEntry.name);
                fs.writeFileSync(fullResultPath, resultSvg);
            }
        }
        dirEntry = inDir.readSync();
    }
}
;
async function getImageData(svgString, renderScaleFactor) {
    const jsdom = require("jsdom");
    const window = new jsdom.JSDOM().window;
    const parser = new window.DOMParser();
    const serializer = new window.XMLSerializer();
    const svgDoc = parser.parseFromString(svgString, 'image/svg+xml');
    const svgElement = svgDoc.documentElement;
    const svgWidth = parseFloat(svgElement.getAttribute('width'));
    const svgHeight = parseFloat(svgElement.getAttribute('height'));
    const scale = Math.min(width / svgWidth) * renderScaleFactor;
    const scaledHeight = svgHeight * scale;
    const scaledWidth = svgWidth * scale;
    svgElement.setAttribute('width', scaledWidth.toString());
    svgElement.setAttribute('height', scaledHeight.toString());
    const scaledSvgString = serializer.serializeToString(svgElement);
    const image = await (0, canvas_1.loadImage)(`data:image/svg+xml;base64,${btoa(scaledSvgString)}`);
    const canvas = (0, canvas_1.createCanvas)(scaledWidth, scaledHeight);
    const ctx = canvas.getContext('2d');
    // Draw the image onto the canvas
    ctx.drawImage(image, 0, 0, scaledWidth, scaledHeight);
    // Get the ImageData from the canvas
    const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
    const dataMap = new Map();
    for (const val of imageData.data) {
        if (!dataMap.has(val)) {
            dataMap.set(val, 1);
        }
        else {
            dataMap.set(val, dataMap.get(val) + 1);
        }
    }
    const kvps = Array.from(dataMap);
    kvps.sort((a, b) => b[1] - a[1]);
    const fullImageData = { ...imageData, colorSpace: "srgb", height: canvas.height, width: canvas.width };
    return [fullImageData, svgWidth, svgHeight];
}
async function main_pathTracer() {
    const dirPath = path_1.default.join(__dirname, '../svgs');
    const inDir = fs.opendirSync(dirPath);
    const outDirPath = path_1.default.join(__dirname, '../svgs/out/');
    let dirEntry = inDir.readSync();
    while (dirEntry) {
        if (dirEntry.isFile() && dirEntry.name.endsWith(".svg")) {
            if (dirEntry.name == "finitecurve.svg") {
                console.log(`processing ${dirEntry.name}`);
                const file = fs.readFileSync(path_1.default.join(dirEntry.path, dirEntry.name));
                const svgString = file.toString();
                const jsdom = require("jsdom");
                const window = new jsdom.JSDOM().window;
                const parser = new window.DOMParser();
                const svgDoc = parser.parseFromString(svgString, 'image/svg+xml');
                const svgElement = svgDoc.documentElement;
                const svgWidth = parseFloat(svgElement.getAttribute('width'));
                const svgHeight = parseFloat(svgElement.getAttribute('height'));
                const height = Math.floor(svgHeight * (width / svgWidth));
                const svgJson = convertSvgToSvgJson(svgString);
                const request = {
                    svgJson: svgJson,
                    height,
                    width,
                    svgWidth,
                    svgHeight,
                    homeX: 0,
                    homeY: 0,
                    infillDensity: 0,
                    type: 'renderSvg',
                    flattenPaths: false,
                };
                const result = await (0, toCommands_1.renderSvgJsonToCommands)(request, updater);
                fs.writeFileSync(path_1.default.join(__dirname, '../svgs/out/commands.txt'), result.commands.join('\n'));
                const resultSvgJsonString = (0, toSvgJson_1.renderCommandsToSvgJson)(result.commands, width, height, updater);
                const resultSvg = convertSvgJsonToSvg(resultSvgJsonString, width, height);
                const fullResultPath = path_1.default.join(outDirPath, dirEntry.name);
                fs.writeFileSync(fullResultPath, resultSvg);
            }
        }
        dirEntry = inDir.readSync();
    }
}
function convertSvgToSvgJson(svgString) {
    const size = new paper.Size(Number.MAX_SAFE_INTEGER, Number.MAX_SAFE_INTEGER);
    paper.setup(size);
    const svg = paper.project.importSVG(svgString, {
        expandShapes: true,
        applyMatrix: true,
    });
    const json = svg.exportJSON();
    paper.project.remove();
    return json;
}
function convertSvgJsonToSvg(svgJson, width, height) {
    const size = new paper.Size(width, height);
    paper.setup(size);
    paper.project.importJSON(svgJson);
    const svg = paper.project.exportSVG({
        asString: true,
    });
    paper.project.remove();
    return svg;
}
main_pathTracer();
//# sourceMappingURL=tester.js.map