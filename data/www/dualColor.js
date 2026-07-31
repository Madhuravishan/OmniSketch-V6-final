// =====================================================================
//  OMNISKETCH (Step 3): dual-color command merger.
//
//  Takes two command outputs from the existing single-color renderer
//  (one per SVG / pen) and produces a single unified command file that
//  the firmware (Runner) understands as a dual-pen drawing.
//
//  Input format (each layer, from the worker):
//      d<distance>
//      h<height>
//      <body: p0 / p1 / "x y" lines>
//
//  Output format (merged):
//      d<sum of both distances>
//      h<height>
//      t0                  <- select pen 1 (firmware raises both pens)
//      <body of layer 1>
//      p0                  <- defensive: ensure pen 1 is up before swap
//      t1                  <- select pen 2 (firmware raises both pens)
//      p0                  <- defensive: ensure pen 2 is up before first move
//      <body of layer 2>
//      p0                  <- defensive: ensure pen 2 is up at end
//
//  Notes:
//    - Both SVGs MUST share the same viewBox. If they don't, the two
//      color layers won't register on the wall.
//    - The pen-2 physical tip offset is handled in firmware (config.h:
//      PEN2_D_P_MM via the kinematic solver), not by translating
//      coordinates here.
//
//  OMNISKETCH (post-3c) fix - pen drag during tool change:
//  Originally the merger relied on each layer's own ending p0. But the
//  upstream worker only emits p0 after the LAST stroke - if a layer
//  ends mid-pen-down it could still leave pen 1 touching the wall
//  during the t1 swap, dragging across the surface. We now insert
//  explicit p0 (pen up) commands around every tool change to make the
//  sequence pen-safe regardless of what the layer body looks like:
//      ...layer 1... -> p0 -> t1 -> p0 -> ...layer 2... -> p0
//  Extra p0's are cheap (no-op if pen is already up).
// =====================================================================

export function mergeLayers(layer1Text, layer2Text) {
    const lines1 = layer1Text.split('\n');
    const lines2 = layer2Text.split('\n');

    if (lines1.length < 2 || lines1[0][0] !== 'd' || lines1[1][0] !== 'h') {
        throw new Error('Layer 1: missing or malformed d/h header');
    }
    if (lines2.length < 2 || lines2[0][0] !== 'd' || lines2[1][0] !== 'h') {
        throw new Error('Layer 2: missing or malformed d/h header');
    }

    const d1 = parseFloat(lines1[0].substring(1));
    const d2 = parseFloat(lines2[0].substring(1));
    const h1 = lines1[1];
    const h2 = lines2[1];

    if (h1 !== h2) {
        console.warn(
            `dualColor.mergeLayers: layer heights differ (${h1} vs ${h2}). ` +
            `The two SVGs may not have the same viewBox - registration on ` +
            `the wall will be off.`
        );
    }

    const body1 = lines1.slice(2);
    const body2 = lines2.slice(2);

    const totalDistance = +(d1 + d2).toFixed(1);

    return [
        `d${totalDistance}`,
        h1,         // both layers share the same drawing area
        't0',       // start on pen 1 (firmware ToolChangeTask raises both pens)
        ...body1,
        'p0',       // safety: ensure pen 1 is up before the tool change move
        't1',       // switch to pen 2 (firmware raises both pens, repositions for pen 2 geometry)
        'p0',       // safety: ensure pen 2 is up before its first stroke
        ...body2,
        'p0',       // safety: ensure pen 2 is up at end (before home)
    ].join('\n');
}

