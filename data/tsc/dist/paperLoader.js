"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.loadPaper = void 0;
const process_1 = require("process");
let loaded = false;
function loadPaper() {
    if (process_1.env && process_1.env["server"]) {
        const paperModule = require("paper");
        return paperModule;
    }
    else {
        if (!loaded) {
            importScripts("https://cdnjs.cloudflare.com/ajax/libs/paper.js/0.12.17/paper-full.min.js");
            self.paper.install(self);
            loaded = true;
        }
        return self.paper;
    }
}
exports.loadPaper = loadPaper;
//# sourceMappingURL=paperLoader.js.map