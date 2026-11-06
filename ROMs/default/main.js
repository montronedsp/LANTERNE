/**
 * Lanterne default ROM — WebGL bar visualizer + HUD. ES5-only for host compatibility.
 */
(function () {
  'use strict';

  var glCanvas = document.getElementById('glCanvas');
  var hudCanvas = document.getElementById('hudCanvas');
  if (!glCanvas || !hudCanvas) return;

  var gl, ctx;
  try {
    if (typeof glCanvas.getContext !== 'function' || typeof hudCanvas.getContext !== 'function') {
      throw new Error('getContext not available');
    }
    gl = glCanvas.getContext('webgl') || glCanvas.getContext('experimental-webgl');
    if (!gl) throw new Error('WebGL not supported');
    ctx = hudCanvas.getContext('2d');
    if (!ctx) throw new Error('2D context not supported');
  } catch (e) {
    document.body.innerHTML = '<div style="color:#fff;font-family:monospace;padding:12px;">Lanterne: ' + (e.message || 'Canvas not supported') + '.</div>';
    return;
  }

  var audioData = null;
  window.__ROMReceiveAudio = function (data) {
    audioData = data;
  };

  /* Settings from host */
  var S = window.__ROMSettings || {};
  var cfgShowFPS = !!S.showFPS;

  window.__ROMOnSettings = function(s) {
    cfgShowFPS = !!s.showFPS;
  };

  /* FPS tracking */
  var _fpsFrames = 0, _fpsLast = performance.now(), _fpsVal = 0;
  function updateFPS() {
    _fpsFrames++;
    var now = performance.now();
    if (now - _fpsLast >= 1000) {
      _fpsVal = Math.round(_fpsFrames * 1000 / (now - _fpsLast));
      _fpsFrames = 0; _fpsLast = now;
    }
  }

  function resize() {
    var w = window.innerWidth;
    var h = window.innerHeight;
    glCanvas.width = w;
    glCanvas.height = h;
    gl.viewport(0, 0, w, h);
    hudCanvas.width = w;
    hudCanvas.height = h;
  }
  window.addEventListener('resize', resize);
  resize();

  var vsSource = [
    '#pragma vscode_glsllint_stage : vert',
    'attribute vec2 aPosition;',
    'uniform vec4 uColor;',
    'void main() {',
    '  gl_Position = vec4(aPosition, 0.0, 1.0);',
    '}'
  ].join('\n');
  var fsSource = [
    '#pragma vscode_glsllint_stage : frag',
    'precision mediump float;',
    'uniform vec4 uColor;',
    'void main() {',
    '  gl_FragColor = uColor;',
    '}'
  ].join('\n');

  function compileShader(gl, type, source) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      console.error(gl.getShaderInfoLog(shader));
      gl.deleteShader(shader);
      return null;
    }
    return shader;
  }

  var vs = compileShader(gl, gl.VERTEX_SHADER, vsSource);
  var fs = compileShader(gl, gl.FRAGMENT_SHADER, fsSource);
  if (!vs || !fs) return;

  var program = gl.createProgram();
  gl.attachShader(program, vs);
  gl.attachShader(program, fs);
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    console.error(gl.getProgramInfoLog(program));
    return;
  }

  gl.useProgram(program);
  var aPositionLoc = gl.getAttribLocation(program, 'aPosition');
  var uColorLoc = gl.getUniformLocation(program, 'uColor');
  gl.enableVertexAttribArray(aPositionLoc);

  var positionBuffer = gl.createBuffer();
  var numBars = 64;
  var fftSize = 256;

  var smoothedBars = new Float32Array(32);

  function drawBottomSpectrum(ctx, w, h) {
    var stripH = 36;
    var maxBarH = 32;
    var barW = w / 32;
    var fft = (audioData && audioData.fft) ? audioData.fft : null;

    ctx.fillStyle = 'rgba(0, 0, 0, 0.5)';
    ctx.fillRect(0, h - stripH, w, stripH);

    for (var i = 0; i < 32; i++) {
      var raw = 0;
      if (fft && fft.length >= (i + 1) * 8) {
        var sum = 0;
        for (var j = 0; j < 8; j++) sum += fft[i * 8 + j];
        raw = sum / 8;
      }
      smoothedBars[i] = 0.25 * raw + 0.75 * smoothedBars[i];
      var val = Math.log(1 + smoothedBars[i] * 9) / Math.log(10);

      var barH = val * maxBarH;
      ctx.fillStyle = 'rgba(0, 180, 120, 0.7)';
      ctx.fillRect(i * barW + 1, h - barH - 2, barW - 2, barH);
    }
  }

  function lerpColor(a, b, t) {
    return [
      a[0] + (b[0] - a[0]) * t,
      a[1] + (b[1] - a[1]) * t,
      a[2] + (b[2] - a[2]) * t,
      1.0
    ];
  }

  function drawBars() {
    var w = glCanvas.width;
    var h = glCanvas.height;
    var barW = (w / numBars) - 1;
    var gap = 1;

    var fft = (audioData && audioData.fft && Array.isArray(audioData.fft)) ? audioData.fft : null;
    var binStep = fftSize / numBars;

    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);

    for (var i = 0; i < numBars; i++) {
      var binValue = 0;
      if (fft && fft.length >= (i + 1) * binStep) {
        for (var k = i * binStep; k < (i + 1) * binStep && k < fft.length; k++) {
          binValue += fft[k];
        }
        binValue /= binStep;
      }
      var barHeight = Math.min(binValue * h, h);

      var t = i / (numBars - 1);
      var color;
      if (t <= 0.5) {
        var s = t * 2;
        color = lerpColor([0, 30/255, 180/255], [0, 200/255, 220/255], s);
      } else {
        var s = (t - 0.5) * 2;
        color = lerpColor([0, 200/255, 220/255], [1, 1, 1], s);
      }
      gl.uniform4fv(uColorLoc, color);

      var x0 = -1 + (2 * i * (barW + gap)) / w;
      var x1 = -1 + (2 * (i * (barW + gap) + barW)) / w;
      var y0 = -1;
      var y1 = -1 + (2 * barHeight) / h;

      var positions = new Float32Array([
        x0, y0, x1, y0, x0, y1,
        x0, y1, x1, y0, x1, y1
      ]);
      gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
      gl.bufferData(gl.ARRAY_BUFFER, positions, gl.DYNAMIC_DRAW);
      gl.vertexAttribPointer(aPositionLoc, 2, gl.FLOAT, false, 0, 0);
      gl.drawArrays(gl.TRIANGLES, 0, 6);
    }
  }

  function drawIdleWave() {
    var w = glCanvas.width;
    var h = glCanvas.height;
    var amplitude = 40;
    var centerY = h / 2;
    var phase = Date.now() * 0.001;
    var cycles = 2;
    var points = 64;

    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);

    var color = [0, 200/255, 220/255, 1];
    gl.uniform4fv(uColorLoc, color);

    for (var i = 0; i < points; i++) {
      var xNorm = i / (points - 1);
      var x = (xNorm * w);
      var y = centerY + amplitude * Math.sin(xNorm * Math.PI * 2 * cycles + phase);
      var x0 = -1 + (2 * (x - 0.5)) / w;
      var x1 = -1 + (2 * (x + 0.5)) / w;
      var y0 = -1 + (2 * (y - 0.5)) / h;
      var y1 = -1 + (2 * (y + 0.5)) / h;
      var positions = new Float32Array([
        x0, y0, x1, y0, x0, y1,
        x0, y1, x1, y0, x1, y1
      ]);
      gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
      gl.bufferData(gl.ARRAY_BUFFER, positions, gl.DYNAMIC_DRAW);
      gl.vertexAttribPointer(aPositionLoc, 2, gl.FLOAT, false, 0, 0);
      gl.drawArrays(gl.TRIANGLES, 0, 6);
    }
  }

  function drawHUD() {
    var w = hudCanvas.width;
    var h = hudCanvas.height;
    ctx.clearRect(0, 0, w, h);
    ctx.font = '13px monospace';
    ctx.fillStyle = 'white';
    ctx.fillText('Lanterne v0.1', 12, 24);
    var rmsStr = (audioData && typeof audioData.rms === 'number')
      ? 'RMS: ' + audioData.rms.toFixed(2)
      : 'RMS: 0.00';
    ctx.fillText(rmsStr, 12, 42);
    ctx.fillText('BPM: --', 12, 60);
    if (cfgShowFPS) ctx.fillText('FPS: ' + _fpsVal, 12, 78);
    drawBottomSpectrum(ctx, w, h);
  }

  function loop() {
    updateFPS();
    if (audioData && audioData.fft && audioData.fft.length > 0 && audioData.rms > 0.001) {
      drawBars();
    } else {
      drawIdleWave();
    }
    drawHUD();
    requestAnimationFrame(loop);
  }
  requestAnimationFrame(loop);
})();
