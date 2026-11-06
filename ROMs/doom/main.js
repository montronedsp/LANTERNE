// SPDX-License-Identifier: MIT
//
// Doom cartridge for Lanterne.
//
// The engine is the doom.wasm v0.1.0 WebAssembly module from
// https://github.com/jacobenget/doom.wasm (GPL-2.0, see engine/LICENSE and
// NOTICE.md). The shareware IWAD is embedded in that binary, so the cartridge
// ships no game data of its own and never touches the network.
//
// The engine talks to this file through a ten-function import object and
// four exports: initGame, tickGame, reportKeyDown, reportKeyUp. Frames come
// back through ui.drawFrame as a 32-bit BGRA framebuffer.

(function () {
  'use strict';

  var TICKS_PER_SECOND = 35;

  // ---------------------------------------------------------------- host bridge

  var audioPayload = {
    rms: 0.0,
    bpm: 0.0,
    bands: { bass: 0.0, mid: 0.0, treble: 0.0 }
  };

  window.__ROMReceiveAudio = function (data) {
    if (data && typeof data === 'object') audioPayload = data;
  };

  var romSettings = { showFPS: false };

  window.__ROMOnSettings = function (settings) {
    if (settings && typeof settings === 'object') {
      romSettings = settings;
      applySettings();
    }
  };

  // ---------------------------------------------------------------- page helpers

  function byId(id) {
    return typeof document !== 'undefined' ? document.getElementById(id) : null;
  }

  function applySettings() {
    var fps = byId('fps');
    if (fps) fps.style.display = romSettings.showFPS ? 'block' : 'none';
  }

  function showError(message) {
    var box = byId('error');
    if (box) {
      box.style.display = 'flex';
      box.textContent = message;
    } else if (typeof console !== 'undefined') {
      console.error(message);
    }
  }

  // atob exists in WebView2 and in Node; no fetch is used anywhere.
  function decodeBase64(text) {
    var binary = atob(text);
    var bytes = new Uint8Array(binary.length);
    for (var i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
    return bytes;
  }

  function readUtf8(memory, pointer, length) {
    var bytes = new Uint8Array(memory.buffer, pointer, length);
    return new TextDecoder('utf-8', { fatal: false }).decode(bytes);
  }

  function scheduleFrame(callback) {
    if (typeof requestAnimationFrame === 'function') {
      requestAnimationFrame(callback);
    } else {
      setTimeout(callback, 1000 / 60);
    }
  }

  // ---------------------------------------------------------------- engine boot

  function boot() {
    var canvas = byId('doomCanvas');
    if (!canvas) {
      showError('Doom cartridge: canvas element is missing.');
      return Promise.resolve(null);
    }

    var encoded = window.__DOOM_WASM_B64;
    if (typeof encoded !== 'string' || encoded.length === 0) {
      showError('Doom cartridge: engine payload is missing (engine/doom.wasm.b64.js).');
      return Promise.resolve(null);
    }

    var context = canvas.getContext('2d');
    var engineMemory = null;
    var scratch = null;
    var drawnFrames = 0;
    var fpsWindowStart = 0;

    function onGameInit(width, height) {
      canvas.width = width;
      canvas.height = height;
      scratch = context.createImageData(width, height);
    }

    function drawFrame(indexOfFrameBuffer) {
      if (!scratch || !engineMemory) return;

      var source = new Uint8Array(
        engineMemory.buffer,
        indexOfFrameBuffer,
        canvas.width * canvas.height * 4
      );
      var target = scratch.data;

      // Engine pixels are 32-bit ARGB stored little-endian, i.e. byte order
      // B, G, R, A; ImageData wants R, G, B, A.
      for (var i = 0; i < target.length; i += 4) {
        target[i] = source[i + 2];
        target[i + 1] = source[i + 1];
        target[i + 2] = source[i];
        target[i + 3] = 255;
      }

      context.putImageData(scratch, 0, 0);
      drawnFrames++;
    }

    function timeInMilliseconds() {
      return BigInt(Math.trunc(performance.now()));
    }

    function onInfoMessage(pointer, length) {
      if (typeof console !== 'undefined') {
        console.log('[Doom] ' + readUtf8(engineMemory, pointer, length));
      }
    }

    function onErrorMessage(pointer, length) {
      if (typeof console !== 'undefined') {
        console.error('[Doom] ' + readUtf8(engineMemory, pointer, length));
      }
    }

    function updateFps(now) {
      if (!romSettings.showFPS) return;

      var label = byId('fps');
      if (!label) return;

      if (fpsWindowStart === 0) {
        fpsWindowStart = now;
        return;
      }

      var span = now - fpsWindowStart;
      if (span >= 500) {
        label.textContent = Math.round((drawnFrames * 1000) / span) + ' fps';
        drawnFrames = 0;
        fpsWindowStart = now;
      }
    }

    var imports = {
      loading: {
        onGameInit: onGameInit,
        // No host-side WAD: the module falls back to the shareware IWAD
        // compiled into the binary.
        wadSizes: function () {},
        readWads: function () {}
      },
      ui: { drawFrame: drawFrame },
      runtimeControl: { timeInMilliseconds: timeInMilliseconds },
      console: {
        onInfoMessage: onInfoMessage,
        onErrorMessage: onErrorMessage
      },
      gameSaving: {
        sizeOfSaveGame: function () { return 0; },
        readSaveGame: function () { return 0; },
        writeSaveGame: function () { return 0; }
      }
    };

    var bytes;
    try {
      bytes = decodeBase64(encoded);
    } catch (error) {
      showError('Doom cartridge: could not decode the engine payload (' + error.message + ').');
      return Promise.resolve(null);
    }

    return WebAssembly.instantiate(bytes, imports)
      .then(function (result) {
        var engine = result.instance.exports;
        engineMemory = engine.memory;

        wireKeyboard(canvas, engine);
        focusCanvas(canvas);

        engine.initGame();
        runLoop(function () {
          engine.tickGame();
        }, updateFps);

        applySettings();
        return engine;
      })
      .catch(function (error) {
        showError('Doom cartridge: the engine failed to start (' + error + ').');
        return null;
      });
  }

  // ---------------------------------------------------------------- frame loop

  function runLoop(tick, eachFrame) {
    var tickLength = 1000 / TICKS_PER_SECOND;
    var last = performance.now();
    var accumulated = 0;

    function frame(now) {
      // requestAnimationFrame passes a timestamp; the setTimeout fallback does not.
      if (typeof now !== 'number') now = performance.now();

      var elapsed = now - last;
      last = now;

      if (elapsed < 0) elapsed = 0;
      if (elapsed > 250) elapsed = 250; // a stall must not cause a catch-up burst

      accumulated += elapsed;

      var ticks = 0;
      while (accumulated >= tickLength && ticks < 8) {
        tick();
        accumulated -= tickLength;
        ticks++;
      }
      if (accumulated > tickLength * 8) accumulated = tickLength * 8;

      if (eachFrame) eachFrame(now);
      scheduleFrame(frame);
    }

    scheduleFrame(frame);
  }

  // ---------------------------------------------------------------- keyboard

  // Keys the engine exposes by name; everything else falls back to ASCII.
  function namedKeys(engine) {
    return {
      ArrowLeft: engine.KEY_LEFTARROW,
      ArrowRight: engine.KEY_RIGHTARROW,
      ArrowUp: engine.KEY_UPARROW,
      ArrowDown: engine.KEY_DOWNARROW,
      ',': engine.KEY_STRAFE_L,
      '.': engine.KEY_STRAFE_R,
      Control: engine.KEY_FIRE,
      ' ': engine.KEY_USE,
      Shift: engine.KEY_SHIFT,
      Tab: engine.KEY_TAB,
      Escape: engine.KEY_ESCAPE,
      Enter: engine.KEY_ENTER,
      Backspace: engine.KEY_BACKSPACE,
      Alt: engine.KEY_ALT
    };
  }

  // Modern aliases layered on top of the 1993 default bindings.
  function aliasKeys(engine) {
    return {
      w: engine.KEY_UPARROW,
      s: engine.KEY_DOWNARROW,
      a: engine.KEY_STRAFE_L,
      d: engine.KEY_STRAFE_R,
      q: engine.KEY_LEFTARROW,
      e: engine.KEY_RIGHTARROW,
      f: engine.KEY_FIRE,
      j: engine.KEY_FIRE
    };
  }

  function doomKeyFor(event, named, aliases) {
    if (Object.prototype.hasOwnProperty.call(named, event.key)) return named[event.key];
    if (typeof event.key === 'string' && event.key.length === 1) {
      var lower = event.key.toLowerCase();
      if (Object.prototype.hasOwnProperty.call(aliases, lower)) return aliases[lower];
      return event.key.charCodeAt(0);
    }
    return null;
  }

  function wireKeyboard(canvas, engine) {
    var named = namedKeys(engine);
    var aliases = aliasKeys(engine);

    function handleDown(event) {
      var key = doomKeyFor(event, named, aliases);
      if (key === null) return;
      event.preventDefault();
      engine.reportKeyDown(key);
    }

    function handleUp(event) {
      var key = doomKeyFor(event, named, aliases);
      if (key === null) return;
      event.preventDefault();
      engine.reportKeyUp(key);
    }

    window.addEventListener('keydown', handleDown, { passive: false });
    window.addEventListener('keyup', handleUp, { passive: false });
    canvas.addEventListener('pointerdown', function () { focusCanvas(canvas); });
  }

  function focusCanvas(canvas) {
    if (typeof canvas.focus === 'function') canvas.focus({ preventScroll: true });
  }

  // ---------------------------------------------------------------- start

  window.__DoomROM = { boot: boot };

  if (typeof document !== 'undefined') {
    if (document.readyState === 'complete') {
      boot();
    } else {
      window.addEventListener('load', boot);
    }
  }
})();
