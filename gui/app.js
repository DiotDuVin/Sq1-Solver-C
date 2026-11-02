'use strict';
/*
 * Visualiseur Square-1 : dessine les couches et rejoue pas à pas les solutions
 * produites par sq1_solver.c (compilé en WebAssembly, voir build.sh).
 *
 * Représentation d'un état (identique au tableau pos[] interne du solveur) :
 * 24 cases de 30° (0-11 : haut, 12-23 : bas) + 1 caractère pour la couche
 * médiane ('-' carré, '/' kite, '?' inconnue). Un coin occupe deux cases.
 *
 * Géométrie (angles en degrés, sens horaire depuis midi, vue de dessus) :
 *  - haut : la case k couvre [180+30k, 210+30k] ;
 *  - bas (vu de dessus, par transparence) : la case k couvre [150-30k, 180-30k].
 * Le trait de coupe est vertical ; le twist échange la moitié droite du haut
 * (cases 6-11) avec celle du bas (cases 12-17) en la retournant.
 */

const SOLVED = 'AA1BB2CC3DD45EE6FF7GG8HH';

// ---------------------------------------------------------------- géométrie
const S = 100;                              // apothème du carré
const R1 = S / Math.cos(Math.PI / 12);      // distance centre → bord d'une arête
const RT = S * Math.SQRT2;                  // distance centre → pointe d'un coin
const K = 0.78;                             // limite intérieure des bandes latérales
const FACES = [[75, '#ff8a1c'], [165, '#2fb350'], [255, '#e5383b'], [345, '#2f74e8']];
const BODY_TOP = '#f6f6f1';
const BODY_BOT = '#ffd43b';

const rad = d => d * Math.PI / 180;
const norm = a => ((a % 360) + 360) % 360;
const P = (a, r) => [r * Math.sin(rad(a)), -r * Math.cos(rad(a))];
const scale = (q, k) => [q[0] * k, q[1] * k];
const isCorner = c => c >= 'A' && c <= 'H';
const mod12 = n => ((n % 12) + 12) % 12;
const shortTurn = t => (t > 6 ? t - 12 : t);

function faceColor(a) {
  let best = FACES[0][1], bd = 1e9;
  for (const [fa, col] of FACES) {
    const d = Math.abs(norm(a - fa + 180) - 180);
    if (d < bd) { bd = d; best = col; }
  }
  return best;
}

/** Pièces d'une couche : {c, corner, angle} (angle = milieu de l'arête ou pointe du coin). */
function layerPieces(state, bottom) {
  const off = bottom ? 12 : 0, out = [];
  for (let k = 0; k < 12; k++) {
    const c = state[off + k];
    if (isCorner(c)) {
      if (state[off + (k + 1) % 12] !== c) continue;          // 2e moitié du coin
      out.push({ c, corner: true, angle: bottom ? 150 - 30 * k : 210 + 30 * k });
    } else {
      out.push({ c, corner: false, angle: bottom ? 165 - 30 * k : 195 + 30 * k });
    }
  }
  return out;
}

// Position d'origine de chaque pièce (sert à colorer les faces latérales)
const HOME = {};
for (const bottom of [false, true])
  for (const p of layerPieces(SOLVED, bottom)) HOME[p.c] = { angle: p.angle, bottom };

const fmt = q => q[0].toFixed(2) + ',' + q[1].toFixed(2);
const poly = (pts, fill, cls) => `<polygon class="${cls}" fill="${fill}" points="${pts.map(fmt).join(' ')}"/>`;
const band = (a, b, col) => poly([a, b, scale(b, K), scale(a, K)], col, 'sticker');

function pieceSVG(p, bottom, rot) {
  const home = HOME[p.c];
  const body = home.bottom ? BODY_BOT : BODY_TOP;
  const th = p.angle;
  let s;
  if (!p.corner) {
    const a = P(th - 15, R1), b = P(th + 15, R1);
    s = poly([[0, 0], a, b], body, 'piece') + band(a, b, faceColor(home.angle));
  } else {
    // une pièce passée dans l'autre couche est retournée : ses deux faces s'échangent
    const flipped = home.bottom !== bottom;
    const a = P(th - 30, R1), v = P(th, RT), b = P(th + 30, R1);
    s = poly([[0, 0], a, v, b], body, 'piece')
      + band(a, v, faceColor(home.angle + (flipped ? 45 : -45)))
      + band(v, b, faceColor(home.angle + (flipped ? -45 : 45)));
  }
  const [x, y] = P(th, p.corner ? RT * 0.5 : R1 * 0.55);
  return s + `<text class="lbl" x="${x.toFixed(1)}" y="${y.toFixed(1)}" transform="rotate(${-rot} ${x.toFixed(1)} ${y.toFixed(1)})">${p.c}</text>`;
}

function layerSVG(state, bottom, rot, flip, showHalf) {
  let left = '', right = '';
  for (const p of layerPieces(state, bottom)) {
    if (Math.sin(rad(p.angle)) > 1e-6) right += pieceSVG(p, bottom, rot);
    else left += pieceSVG(p, bottom, rot);
  }
  const half = showHalf ? '<rect class="half" x="0" y="-150" width="150" height="300"/>' : '';
  return half
    + `<g transform="rotate(${rot})">${left}<g transform="scale(1 ${flip})">${right}</g></g>`
    + '<line class="slice" x1="0" y1="-148" x2="0" y2="148"/>';
}

function middleSVG(ml, flip, showHalf) {
  const left = [P(180, R1), P(210, RT), P(300, RT), P(360, R1)];
  const right = [P(0, R1), P(30, RT), P(120, RT), P(180, R1)];
  const L = poly(left, '#8a8f99', 'midpiece')
    + band(left[0], left[1], faceColor(165)) + band(left[1], left[2], faceColor(255)) + band(left[2], left[3], faceColor(345));
  let R;
  if (ml === '?') {
    R = poly(right, 'none', 'unknown');
  } else {
    R = `<g transform="scale(1 ${(ml === '/' ? -1 : 1) * flip})">`
      + poly(right, '#8a8f99', 'midpiece')
      + band(right[0], right[1], faceColor(345)) + band(right[1], right[2], faceColor(75)) + band(right[2], right[3], faceColor(165))
      + '</g>';
  }
  const half = showHalf ? '<rect class="half" x="0" y="-150" width="150" height="300"/>' : '';
  return half + L + R + '<line class="slice" x1="0" y1="-148" x2="0" y2="148"/>';
}

// ------------------------------------------------------- simulation (JS)
const splitAt = (s, i, j) => isCorner(s[i]) && s[i] === s[j];
const canTwist = s => !splitAt(s, 11, 0) && !splitAt(s, 5, 6) && !splitAt(s, 23, 12) && !splitAt(s, 17, 18);

function turn(s, t, b) {
  const a = s.split('');
  const r = s.split('');
  for (let i = 0; i < 12; i++) {
    r[i] = a[mod12(i - t)];
    r[12 + i] = a[12 + mod12(i - b)];
  }
  return r.join('');
}
function twist(s) {
  const a = s.split('');
  for (let j = 0; j < 6; j++) [a[6 + j], a[12 + j]] = [a[12 + j], a[6 + j]];
  a[24] = a[24] === '-' ? '/' : a[24] === '/' ? '-' : '?';
  return a.join('');
}

/** État interne → format utilisateur (17 caractères), ou null s'il n'est pas aligné. */
function toUser(s) {
  if (splitAt(s, 11, 0) || splitAt(s, 23, 12)) return null;
  let out = '';
  for (let a = 0; a < 24; a++) { out += s[a]; if (isCorner(s[a])) a++; }
  return out + (s[24] === '?' ? '' : s[24]);
}

/** Applique une suite comme "(1,0)/(-3,3)/" à l'état résolu. */
function parseScramble(str) {
  const re = /\s*(?:\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)|(\/))\s*/y;
  let s = SOLVED + '-', i = 0;
  const steps = [];
  str = str.trim();
  while (i < str.length) {
    re.lastIndex = i;
    const m = re.exec(str);
    if (!m || re.lastIndex === i) throw new Error(`notation invalide près de « ${str.slice(i, i + 10)} »`);
    i = re.lastIndex;
    if (m[3]) {
      if (!canTwist(s)) throw new Error(`twist impossible après le mouvement ${steps.length} (un coin est à cheval sur la coupe)`);
      s = twist(s);
      steps.push({ label: '/', twist: true, top: 0, bottom: 0, state: s, phase: 'scramble' });
    } else {
      const t = mod12(+m[1]), b = mod12(+m[2]);
      s = turn(s, t, b);
      steps.push({ label: `(${shortTurn(t)},${shortTurn(b)})`, twist: false, top: t, bottom: b, state: s, phase: 'scramble' });
    }
  }
  if (!steps.length) throw new Error('mélange vide');
  return { steps, final: s };
}

/** Mélange aléatoire valide de `n` twists, au format "(a,b)/(c,d)/…". */
function randomScramble(n = 12) {
  let s = SOLVED + '-', out = '';
  for (let i = 0; i < n; i++) {
    let t, b, next;
    do {
      t = Math.floor(Math.random() * 12) - 5;
      b = Math.floor(Math.random() * 12) - 5;
      next = turn(s, mod12(t), mod12(b));
    } while ((t === 0 && b === 0 && i > 0) || !canTwist(next));
    s = twist(next);
    out += `(${t},${b})/`;
  }
  return out;
}

// ------------------------------------------------------------ WebAssembly
let wasm = null;
const enc = new TextEncoder(), dec = new TextDecoder();

async function loadWasm() {
  const bin = Uint8Array.from(atob(window.SQ1_WASM_BASE64), c => c.charCodeAt(0));
  const { instance } = await WebAssembly.instantiate(bin, {});
  wasm = instance.exports;
  wasm.sq1_init();
}
const memory = () => new Uint8Array(wasm.memory.buffer);
const readOut = n => dec.decode(memory().subarray(wasm.sq1_outbuf(), wasm.sq1_outbuf() + n));

function wasmSolve(pos) {
  const bytes = enc.encode(pos.slice(0, 40) + '\0');
  memory().set(bytes, wasm.sq1_inbuf());
  return JSON.parse(readOut(wasm.sq1_solve()));
}

// ----------------------------------------------------------------- état UI
const $ = id => document.getElementById(id);
const ui = {
  pos: $('pos'), scramble: $('scramble'), status: $('status'),
  top: $('svg-top'), mid: $('svg-mid'), bot: $('svg-bot'), midLabel: $('mid-label'),
  stepCount: $('step-count'), nextMove: $('next-move'), stateStr: $('state-str'),
  play: $('btn-play'), first: $('btn-first'), prev: $('btn-prev'), next: $('btn-next'), last: $('btn-last'),
  timeline: $('timeline'), speed: $('speed'), moves: $('moves'),
};

let states = [SOLVED + '-'];   // states[i] = état après i étapes
let steps = [];                // steps[i] fait passer de states[i] à states[i+1]
let cur = 0;
let anim = null;               // id requestAnimationFrame
let playing = false;

function draw(state, o = {}) {
  const flip = o.flip ?? 1, half = !!o.half;
  ui.top.innerHTML = layerSVG(state, false, o.top || 0, flip, half);
  ui.bot.innerHTML = layerSVG(state, true, o.bot || 0, flip, half);
  ui.mid.innerHTML = middleSVG(state[24], flip, half);
  ui.midLabel.textContent = state[24] === '-' ? '(carré)' : state[24] === '/' ? '(kite)' : '(inconnue)';
}

function refresh() {
  const n = steps.length;
  const nextStep = steps[cur];
  draw(states[cur], { half: !!(nextStep && nextStep.twist) });
  ui.stepCount.textContent = `${cur} / ${n}`;
  ui.nextMove.textContent = nextStep ? nextStep.label : (n ? 'terminé ✓' : '—');
  const u = toUser(states[cur]);
  ui.stateStr.textContent = u || states[cur] + ' (non aligné)';
  ui.timeline.max = n;
  ui.timeline.value = cur;
  ui.prev.disabled = ui.first.disabled = cur === 0;
  ui.next.disabled = ui.last.disabled = cur === n;
  ui.play.textContent = playing ? '⏸' : '▶';
  ui.play.disabled = n === 0;
  ui.moves.querySelectorAll('.chip').forEach(ch => {
    const i = +ch.dataset.i;
    ch.classList.toggle('active', i === cur);
    ch.classList.toggle('done', i < cur);
  });
  const active = ui.moves.querySelector('.chip.active');
  if (active && playing) active.scrollIntoView({ block: 'nearest', inline: 'nearest' });
}

function buildChips() {
  const parts = [`<button class="chip start" data-i="0">départ</button>`];
  steps.forEach((s, i) => parts.push(`<button class="chip ${s.phase}" data-i="${i + 1}" title="${s.phase === 'scramble' ? 'mélange' : s.phase === 'p1' ? 'phase 1' : 'phase 2'}">${s.label}</button>`));
  ui.moves.innerHTML = parts.join('');
}

function stopAnim() {
  if (anim) cancelAnimationFrame(anim);
  anim = null;
}

function jump(i) {
  stopAnim();
  cur = Math.max(0, Math.min(steps.length, i));
  refresh();
}

const ease = p => (p < 0.5 ? 2 * p * p : 1 - Math.pow(-2 * p + 2, 2) / 2);

/** Anime une étape vers l'avant (+1) ou l'arrière (-1). */
function animate(dir, done) {
  stopAnim();
  const target = cur + dir;
  if (target < 0 || target > steps.length) { done && done(false); return; }
  const st = steps[dir > 0 ? cur : target];
  const from = states[cur], to = states[target];
  const dur = +ui.speed.value * (st.twist ? 0.8 : 1);
  const t0 = performance.now();
  const frame = now => {
    const p = Math.min(1, (now - t0) / dur), e = ease(p);
    if (st.twist) {
      const c = Math.cos(Math.PI * e);
      if (e < 0.5) draw(from, { flip: c, half: true });
      else draw(to, { flip: -c, half: true });
    } else {
      draw(from, { top: dir * shortTurn(st.top) * 30 * e, bot: -dir * shortTurn(st.bottom) * 30 * e });
    }
    if (p < 1) { anim = requestAnimationFrame(frame); return; }
    anim = null;
    cur = target;
    refresh();
    done && done(true);
  };
  anim = requestAnimationFrame(frame);
}

function play() {
  if (!steps.length) return;
  if (cur >= steps.length) cur = 0;
  playing = true;
  refresh();
  const loop = () => {
    if (!playing) return;
    if (cur >= steps.length) { playing = false; refresh(); return; }
    animate(1, ok => { if (ok && playing) setTimeout(loop, +ui.speed.value * 0.25); });
  };
  loop();
}
function pause() {
  playing = false;
  stopAnim();
  refresh();
}

function setStatus(html, error = false) {
  ui.status.innerHTML = html;
  ui.status.classList.toggle('error', error);
}
const esc = s => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

/** Résout `pos` ; `prefix` = étapes de mélange affichées avant la solution. */
function solve(pos, prefix = [], startState = null, note = '') {
  pause();
  const t0 = performance.now();
  const r = wasmSolve(pos);
  const ms = performance.now() - t0;
  if (!r.ok) { setStatus(esc(r.error), true); return; }

  let twists = 0;
  const sol = r.steps.map(s => {
    const phase = twists < r.phase1 ? 'p1' : 'p2';
    if (s.twist) twists++;
    return { ...s, phase };
  });
  steps = prefix.concat(sol);
  states = [startState || r.start, ...steps.map(s => s.state)];
  buildChips();
  cur = 0;

  const solStr = sol.map(s => s.label).join('') || '(déjà résolu)';
  setStatus(`${note}Solution en <b>${r.twists} twists</b> (phase 1 : ${r.phase1}, phase 2 : ${r.twists - r.phase1})`
    + ` trouvée en ${ms < 1 ? '<1' : ms.toFixed(0)} ms — ${r.nodes1.toLocaleString('fr')} + ${r.nodes2.toLocaleString('fr')} nœuds explorés`
    + `<br><code>${esc(solStr)}</code>`);
  refresh();
}

// ------------------------------------------------------------- événements
$('btn-solve').onclick = () => solve(ui.pos.value.trim());
ui.pos.addEventListener('keydown', e => { if (e.key === 'Enter') solve(ui.pos.value.trim()); });

$('btn-random').onclick = () => {
  // Position bien mélangée : 40 twists aléatoires valides appliqués à l'état résolu
  // (le générateur RandomPosition du solveur C mélange beaucoup moins).
  const pos = toUser(parseScramble(randomScramble(40)).final);
  ui.pos.value = pos;
  solve(pos, [], null, `Position aléatoire : <code>${pos}</code><br>`);
};

const doScramble = () => {
  try {
    const { steps: sc, final } = parseScramble(ui.scramble.value);
    const pos = toUser(final);
    if (!pos) throw new Error('la position finale doit être alignée (aucun coin à cheval sur la coupe)');
    ui.pos.value = pos;
    solve(pos, sc, SOLVED + '-', `Mélange de ${sc.length} étapes → <code>${pos}</code><br>`);
  } catch (err) {
    setStatus('Mélange : ' + esc(err.message), true);
  }
};
$('btn-scramble').onclick = doScramble;
$('btn-gen').onclick = () => { ui.scramble.value = randomScramble(); doScramble(); };
ui.scramble.addEventListener('keydown', e => { if (e.key === 'Enter') doScramble(); });

ui.play.onclick = () => (playing ? pause() : play());
ui.first.onclick = () => { pause(); jump(0); };
ui.last.onclick = () => { pause(); jump(steps.length); };
ui.next.onclick = () => { pause(); animate(1); };
ui.prev.onclick = () => { pause(); animate(-1); };
ui.timeline.oninput = () => { pause(); jump(+ui.timeline.value); };
ui.moves.onclick = e => {
  const ch = e.target.closest('.chip');
  if (!ch) return;
  pause();
  const i = +ch.dataset.i;
  if (i === cur + 1) animate(1); else if (i === cur - 1) animate(-1); else jump(i);
};

document.addEventListener('keydown', e => {
  if (e.target.matches('input, select, textarea')) return;
  if (e.key === 'ArrowRight') { e.preventDefault(); pause(); animate(1); }
  else if (e.key === 'ArrowLeft') { e.preventDefault(); pause(); animate(-1); }
  else if (e.key === ' ') { e.preventDefault(); playing ? pause() : play(); }
  else if (e.key === 'Home') { pause(); jump(0); }
  else if (e.key === 'End') { pause(); jump(steps.length); }
});

// ------------------------------------------------------------- démarrage
refresh();
loadWasm()
  .then(() => solve(ui.pos.value.trim()))
  .catch(err => setStatus('Impossible de charger le solveur WebAssembly : ' + esc(err.message), true));
