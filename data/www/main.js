import * as svgControl from './svgControl.js';
import * as client from './client.js';
import * as dualColor from './dualColor.js';   // OMNISKETCH (Step 3)

let currentState = null;

let currentWorker = null;

window.onload = function () {
    init();
};

let uploadConvertedCommands = null;

// OMNISKETCH (Step 3): dual-color drawing state
let isDualColorMode = false;
let layer1Commands = null;

// =====================================================================
//  OMNISKETCH (Step 4): bounding-box utilities for the laser preview.
//
//  We scan the final command string (after any dual-color merge) for
//  coordinate lines, compute the (minX, minY, maxX, maxY) box, and
//  inject a "b<minX> <minY> <maxX> <maxY>" header line right after the
//  d/h header. The firmware (Runner) reads this so the Preview Boundary
//  button can trace the rectangle with the laser on.
// =====================================================================
function computeBoundingBox(commandsText) {
    const lines = commandsText.split('\n');
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (const line of lines) {
        if (line.length === 0) continue;
        const c = line[0];
        // skip header / non-coordinate lines
        if (c === 'd' || c === 'h' || c === 'b' || c === 'p' || c === 't') continue;
        const sp = line.indexOf(' ');
        if (sp <= 0) continue;
        const x = parseFloat(line.substring(0, sp));
        const y = parseFloat(line.substring(sp + 1));
        if (isNaN(x) || isNaN(y)) continue;
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }
    if (minX === Infinity) return null;
    return { minX, minY, maxX, maxY };
}

function prependBoundingBox(commandsText) {
    const bbox = computeBoundingBox(commandsText);
    if (!bbox) return commandsText;
    const bLine = `b${bbox.minX.toFixed(1)} ${bbox.minY.toFixed(1)} ${bbox.maxX.toFixed(1)} ${bbox.maxY.toFixed(1)}`;
    const lines = commandsText.split('\n');
    // Insert immediately after the d (line 0) and h (line 1) headers.
    return [lines[0], lines[1], bLine, ...lines.slice(2)].join('\n');
}

async function checkIfExtendedToHome(extendToHomeTime) {
    await new Promise(r => setTimeout(r, extendToHomeTime * 1000));

    const waitPeriod = 2000;
    let done = false;
    while (!done) {
        try {
            const state = await $.get("/getState");
            if (state.phase !== 'ExtendToHome') {
                adaptToState(state);
                done = true;
            } else {
                await new Promise(r => setTimeout(r, waitPeriod));
            }
        } catch (err) {
            alert("Failed to get current phase: " + err);
            location.reload();
        }
    }
}

function init() {
    function doneWithPhase(custom) {
        $(".muralSlide").hide();
        $("#loadingSlide").show();
        if (!custom) {
            custom = {
                url: "/doneWithPhase",
                data: {},
                commandName: "Done With Phase",
            };
        }

        $.post(custom.url, custom.data || {}, function(state) {
            adaptToState(state);
        }).fail(function() {
            alert(`${custom.commandName} command failed`);
            location.reload();
        });
    }

    $("#beltsRetracted").click(async function() { 
        await client.leftRetractUp();
        await client.rightRetractUp();
        doneWithPhase();
    });

    $("#setDistance").click(function() {
        const inputValue = parseInt($("#distanceInput").val());
        if (isNaN(inputValue)) {
            throw new Error("input value is not a number");
        }

        doneWithPhase({
            url: "/setTopDistance",
            data: {distance: inputValue},
            commandName: "Set Top Distance",
        });
    });

    $("#leftMotorToggle").change(function() {
        if (this.checked) {
            client.leftRetractDown(); 
        } else {
            client.leftRetractUp();
        }
    });

    $("#rightMotorToggle").change(function() {
        if (this.checked) {
            client.rightRetractDown(); 
        } else {
            client.rightRetractUp();
        }
    });

    $("#extendToHome").click(function() {
        $(this).prop( "disabled", true);
        $("#extendingSpinner").css('visibility', 'visible');
        $.post("/extendToHome", {})
        .always(async function(res) {
            const extendToHomeTime = parseInt(res);
            await checkIfExtendedToHome(extendToHomeTime);
        });
    });
    
    // ---------- Pen 1 calibration ----------
    function getServoValueFromInputValue() {
        const inputValue = parseInt($("#servoRange").val());
        const value = 90 - inputValue;
        if (value < 0) return 0;
        if (value > 90) return 90;
        return value;
    }

    $("#servoRange").on('input', $.throttle(250, function (e) {
        const servoValue = getServoValueFromInputValue();
        $.post("/setServo", {angle: servoValue});
    }));

    const stepVaule = 5;
    $("#penMinus").click(function() {
        $("#servoRange")[0].stepDown(stepVaule);
        $("#servoRange").trigger('input');
    });

    $("#penPlus").click(function() {
        $("#servoRange")[0].stepUp(stepVaule);
        $("#servoRange").trigger('input');
    });

    $("#setPenDistance").click(function () {
        const inputValue = getServoValueFromInputValue();
        doneWithPhase({
            url: "/setPenDistance",
            data: {angle: inputValue},
            commandName: "Set Pen Distance",
        });
    });

    // OMNISKETCH (Step 3b): Pen 2 calibration - mirrors pen 1 handlers
    function getServo2ValueFromInputValue() {
        const inputValue = parseInt($("#servoRange2").val());
        const value = 90 - inputValue;
        if (value < 0) return 0;
        if (value > 90) return 90;
        return value;
    }

    $("#servoRange2").on('input', $.throttle(250, function (e) {
        const servoValue = getServo2ValueFromInputValue();
        $.post("/setServo", {angle: servoValue});
    }));

    $("#penMinus2").click(function() {
        $("#servoRange2")[0].stepDown(stepVaule);
        $("#servoRange2").trigger('input');
    });

    $("#penPlus2").click(function() {
        $("#servoRange2")[0].stepUp(stepVaule);
        $("#servoRange2").trigger('input');
    });

    $("#setPenDistance2").click(function () {
        const inputValue = getServo2ValueFromInputValue();
        doneWithPhase({
            url: "/setPenDistance",
            data: {angle: inputValue},
            commandName: "Set Pen 2 Distance",
        });
    });

    $("#skipPen2").click(function() {
        doneWithPhase();
    });

    // OMNISKETCH (Step 3): dual-color mode toggle
    $("#dualColorMode").change(function() {
        isDualColorMode = $(this).is(":checked");
        layer1Commands = null;
        updateDualColorIndicator();
    });

    function updateDualColorIndicator() {
        if (!isDualColorMode) {
            $("#dualColorStatus").hide();
            return;
        }
        if (layer1Commands === null) {
            $("#dualColorStatus")
                .text("Two-color mode: upload PEN 1 SVG (step 1 of 2)")
                .show();
        } else {
            $("#dualColorStatus")
                .text("Two-color mode: upload PEN 2 SVG (step 2 of 2)")
                .show();
        }
    }

    async function getUploadedSvgString() {
        const [file] = $("#uploadSvg")[0].files;
        if (file) {
            return await file.text();
        } else {
            return null;
        }
    }

    $("#uploadSvg").change(async function() {
        const svgString = await getUploadedSvgString();
        if (svgString) {
            svgControl.setSvgString(svgString, currentState);

            $(".svg-control").show();
            $("#preview").removeAttr("disabled");
        } else {
            $("#preview").attr("disabled", "disabled");
            $(".svg-control").hide();
            $("#infillDensity").val(0);
            $("#turdSize").val(2);
        }
    });

    
    let currentPreviewId = 0;
    let rendererFn = null;

    async function render_VectorRasterVector() {
        if (currentWorker) {
            console.log("Terminating previous worker");
            currentWorker.terminate();
        }
        currentPreviewId++;
        const thisPreviewId = currentPreviewId;

        const svgString = await getUploadedSvgString();
        if (!svgString) {
            throw new Error('No SVG string');
        }

        $("#progressBar").text("Rasterizing");
        const raster = await svgControl.getCurrentSvgImageData();

        const vectorizeRequest = {
            type: 'vectorize',
            raster,
            turdSize: getTurdSize(),
        };

        if (currentPreviewId == thisPreviewId) {
            currentWorker = new Worker(`./worker/worker.js?v=${Date.now()}`);

            currentWorker.onmessage = (e) => {
                if (e.data.type === 'status') {
                    $("#progressBar").text(e.data.payload);
                } else if (e.data.type === 'vectorizer') {
                    const vectorizedSvg = e.data.payload.svg;
                    const scale = svgControl.getRenderScale();
                    renderSvgInWorker(
                        currentWorker,
                        vectorizedSvg,
                        svgControl.getTargetWidth() * scale,
                        svgControl.getTargetHeight() * scale,
                    );
                }
                else if (e.data.type === 'log') {
                    console.log(`Worker: ${e.data.payload}`);
                }
            }

            currentWorker.postMessage(vectorizeRequest);
        }
    }

    async function render_PathTracing() {
        if (currentWorker) {
            console.log("Terminating previous worker");
            currentWorker.terminate();
        }
        currentPreviewId++;
        const thisPreviewId = currentPreviewId;

        const svgString = await getUploadedSvgString();
        if (!svgString) {
            throw new Error('No SVG string');
        }

        if (currentPreviewId == thisPreviewId) {
            currentWorker = new Worker(`./worker/worker.js?v=${Date.now()}`);
            currentWorker.onmessage = (e) => {
                if (e.data.type === 'status') {
                    $("#progressBar").text(e.data.payload);
                }
                else if (e.data.type === 'log') {
                    console.log(`Worker: ${e.data.payload}`);
                }
            }

            const renderSvg = svgControl.getRenderSvg();
            const renderSvgString = new XMLSerializer().serializeToString(renderSvg);
            renderSvgInWorker(currentWorker, renderSvgString, svgControl.getTargetWidth(), svgControl.getTargetHeight());
        }
    }

    function renderSvgInWorker(worker, svg, svgWidth, svgHeight) {
        const svgJson = svgControl.getSvgJson(svg);
       
        const renderRequest = {
            type: "renderSvg",
            svgJson,
            width: svgControl.getTargetWidth(),
            height: svgControl.getTargetHeight(),
            svgWidth,
            svgHeight,
            homeX: currentState.homeX,
            homeY: currentState.homeY,
            infillDensity: getInfillDensity(),
            flattenPaths: getFlattenPaths(),
        }

        worker.onmessage = (e) => {
            if (e.data.type === 'status') {
                $("#progressBar").text(e.data.payload);
            } else if (e.data.type === 'renderer') {
                console.log("Worker finished!");

                uploadConvertedCommands = e.data.payload.commands.join('\n');
                const resultSvgJson = e.data.payload.svgJson;
                const resultDataUrl = svgControl.convertJsonToDataURL(resultSvgJson, svgControl.getTargetWidth(), svgControl.getTargetHeight());

                const totalDistanceM = +(e.data.payload.distance / 1000).toFixed(1);
                const drawDistanceM = +(e.data.payload.drawDistance / 1000).toFixed(1);
                
                deactivateProgressBar();
                $("#previewSvg").attr("src", resultDataUrl);
                $("#distances").text(`Total: ${totalDistanceM}m / Draw: ${drawDistanceM}m`);
                $(".svg-preview").show();
                $("#acceptSvg").removeAttr("disabled");
            }
        };

        worker.postMessage(renderRequest);
    }

    function activateProgressBar() {
        const bar = $("#progressBar");
        bar.addClass("progress-bar-striped");
        bar.addClass("progress-bar-animated");
        bar.removeClass("bg-success");
        bar.text("");
    }

    function deactivateProgressBar() {
        const bar = $("#progressBar");
        bar.removeClass("progress-bar-striped");
        bar.removeClass("progress-bar-animated");
        bar.addClass("bg-success");
        bar.text("Success");
    }


    $("#infillDensity,#turdSize,#flattenPathsCheckbox").on('input change', async function() {
        activateProgressBar();
        $("#acceptSvg").attr("disabled", "disabled");
        await rendererFn();
    });

    $("#preview").click(async function() {
        $("#svgUploadSlide").hide();
        $("#chooseRendererSlide").show();
    });

    $("#pathTracing").click(async function() {
        $("label[for='turdSize'],#turdSize").hide();
        $("label[for='flattenPathsCheckbox'],#flattenPathsCheckbox").show();

        $("#chooseRendererSlide").hide();
        $("#drawingPreviewSlide").show();
        rendererFn = render_PathTracing;
        await rendererFn();
    });

    $("#vectorRasterVector").click(async function() {
        $("#flattenPathsCheckbox").prop("checked", false);
        $("label[for='turdSize'],#turdSize").show();
        $("label[for='flattenPathsCheckbox'],#flattenPathsCheckbox").hide();

        $("#chooseRendererSlide").hide();
        $("#drawingPreviewSlide").show();
        rendererFn = render_VectorRasterVector;
        await rendererFn();
    });

    $(".backToSvgSelect").click(function() {
        uploadConvertedCommands = null;

        if (isDualColorMode) {
            layer1Commands = null;
            updateDualColorIndicator();
        }

        $(".loading").show();
        activateProgressBar();
        $("#previewSvg").removeAttr("src");
        $(".svg-preview").hide();
        $("#acceptSvg").attr("disabled", "disabled");

        $("#svgUploadSlide").show();
        $("#drawingPreviewSlide").hide();
        $("#chooseRendererSlide").hide();
    });
    
    $("#acceptSvg").click(function() {
        if (!uploadConvertedCommands) {
            throw new Error('Commands are empty');
        }
        $("#acceptSvg").attr("disabled", "disabled");

        // Dual-color: capture layer 1 and go back for layer 2.
        if (isDualColorMode && layer1Commands === null) {
            layer1Commands = uploadConvertedCommands;
            uploadConvertedCommands = null;

            $("#uploadSvg").val("");
            $(".svg-control").hide();
            $("#preview").attr("disabled", "disabled");
            $("#previewSvg").removeAttr("src");
            $(".svg-preview").hide();
            $("#drawingPreviewSlide").hide();
            $("#chooseRendererSlide").hide();
            $("#svgUploadSlide").show();
            $("#acceptSvg").removeAttr("disabled");

            updateDualColorIndicator();
            return;
        }

        // Compute the final commands string: merged for dual-color, raw for single.
        let commandsToUpload = uploadConvertedCommands;
        if (isDualColorMode && layer1Commands !== null) {
            commandsToUpload = dualColor.mergeLayers(layer1Commands, uploadConvertedCommands);
        }

        // OMNISKETCH (Step 4): inject the bounding-box header for the laser
        // preview. Done after any dual-color merge so the box covers both
        // layers' coordinates.
        commandsToUpload = prependBoundingBox(commandsToUpload);

        // Keep this in sync so verifyUpload compares the actual payload.
        uploadConvertedCommands = commandsToUpload;

        const commandsBlob = new Blob([commandsToUpload], {
            type: "text/plain"
        });

        $(".muralSlide").hide();
        $("#uploadProgress").show();

        const formData = new FormData();
        formData.append("commands", commandsBlob);

        $.ajax({
            url: "/uploadCommands",
            data: formData,
            processData: false,
            contentType: false,
            type: 'POST',
            success: function(data) {
                verifyUpload(data);
            },
            error: function(err) {
                alert('Upload to Mural failed! ' + err);
                window.location.reload();
            },
            xhr: function () {
                var xhr = new window.XMLHttpRequest();

                xhr.upload.addEventListener("progress", function (evt) {
                    if (evt.lengthComputable) {
                        var percentComplete = evt.loaded / evt.total;
                        percentComplete = parseInt(percentComplete * 100);
                        $("#uploadProgress").attr("aria-valuemax", evt.total.toString());
                        $("#uploadProgress").attr("aria-valuenow", evt.loaded.toString());
                        $("#uploadProgress > .progress-bar").attr("style", `width: ${percentComplete}%`);
                    }
                }, false);

                return xhr;
            },
        });
    });



    $("#beginDrawing").click(function() {
        $(".muralSlide").hide();
        $("#drawingBegan").show();
        $.post("/run", {});
    });

    // OMNISKETCH (Step 4): manual laser boundary preview.
    // Fires /previewBoundary on the firmware which runs a single
    // BoundaryTraceTask and stops. The bot returns to the same
    // BeginDrawing screen so the user can then press Begin Drawing.
    $("#previewBoundary").click(function() {
        $("#previewBoundary").attr("disabled", "disabled");
        $.post("/previewBoundary", {})
            .always(function() {
                // Re-enable after a short pause; the firmware is now tracing.
                // We don't poll for completion - user can press Begin when ready.
                setTimeout(function() {
                    $("#previewBoundary").removeAttr("disabled");
                }, 1000);
            });
    });

    $("#reset").click(function() {
        doneWithPhase();
        location.reload();
    });

    $("#leftMotorTool").on('input', function() {
        const leftMotorDir = parseInt($("#leftMotorTool").val());
        if (leftMotorDir <= -1) {
            client.leftRetractDown(); 
        } else if (leftMotorDir >= 1) {
            client.leftExtendDown();
        } else {
            client.leftRetractUp();
        }
    });

    $("#rightMotorTool").on('input', function() {
        const rightMotorDir = parseInt($("#rightMotorTool").val());
        if (rightMotorDir <= -1) {
            client.rightRetractDown(); 
        } else if (rightMotorDir >= 1) {
            client.rightExtendDown();
        } else {
            client.rightRetractUp();
        }
    });

    $("#parkServoTool").click(function() {
        $.post("/setServo", {angle: 0});
    });

    $("#parkServoTool2").click(function() {
        $.post("/parkPen2", {});
    });

    $("#estepsTool").click(function() {
        $.post("/estepsCalibration", {});
    });

    const toolsModal = $("#toolsModal")[0];

    toolsModal.addEventListener('hidden.bs.modal', function (event) {
        client.rightRetractUp();
        client.leftRetractUp();
    });

    svgControl.initSvgControl();

    $("#loadingSlide").show();

    $.get("/getState", function(data) {
        adaptToState(data);
    }).fail(function() {
        alert("Failed to retrieve state");
    });
}

function verifyUpload(state) {
    $.ajax({
            url: "/downloadCommands",
            processData: false,
            contentType: false,
            type: 'GET',
            success: function(data) {
                const receivedData = data.split('\n');
                const sentData = uploadConvertedCommands.split('\n');
                if (receivedData.length !== sentData.length) {
                    alert("Data verification failed");
                    window.location.reload();
                    return;
                }
                for (let i = 0; i < receivedData.length; i++) {
                    if (receivedData[i] !== sentData[i]) {
                        alert("Data verification failed");
                        window.location.reload();
                        return;
                    }
                }
                setTimeout(function() {
                    adaptToState(state);
                }, 1000);
            },
            error: function(err) {
                alert('Failed to download commands from Mural! ' + err);
                window.location.reload();
            },
            xhr: function () {
                var xhr = new window.XMLHttpRequest();
                xhr.addEventListener("progress", function (evt) {
                    if (evt.lengthComputable) {
                        var percentComplete = evt.loaded / evt.total;
                        percentComplete = parseInt(percentComplete * 100);
                        $("#verificationProgress").attr("aria-valuemax", evt.total.toString());
                        $("#verificationProgress").attr("aria-valuenow", evt.loaded.toString());
                        $("#verificationProgress > .progress-bar").attr("style", `width: ${percentComplete}%`);
                    }
                }, false);

                return xhr;
            },
        });
    
}

function adaptToState(state) {
    $(".muralSlide").hide();
    currentState = state;
    switch(state.phase) {
        case "RetractBelts":
            $("#retractBeltsSlide").show();
            break;
        case "SetTopDistance":
            $("#distanceBetweenAnchorsSlide").show();
            break;
        case "ExtendToHome":
            $("#extendToHomeSlide").show();
            if (state.moving || state.startedHoming) {
                $("#extendToHome").prop( "disabled", true);
                $("#extendingSpinner").css('visibility', 'visible');
                checkIfExtendedToHome();
            }
            break;
        case "PenCalibration":
            $.post("/setServo", {angle: 90});
            $("#penCalibrationSlide").show();
            break;
        case "PenCalibration2":
            $.post("/setServo", {angle: 90});
            $("#servoRange2").val(0);
            $("#penCalibration2Slide").show();
            break;
        case "SvgSelect":
            $("#svgUploadSlide").show();
            break;
        case "BeginDrawing":
            $("#beginDrawingSlide").show();
            break;
        default:
            alert("Unrecognized phase");
    }
}

function getInfillDensity() {
    const density = parseInt($("#infillDensity").val());
    if ([0, 1, 2, 3, 4].includes(density)) {
        return density;
    } else {
        throw new Error('Invalid density');
    }
}

function getTurdSize() {
    return parseInt($("#turdSize").val());
}

function getFlattenPaths() {
    return $("#flattenPathsCheckbox").is(":checked");
}

