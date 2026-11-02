'use strict';
/*
 * Vue 3D du Square-1 (three.js r149, script classique dans vendor/).
 *
 * Réutilise la géométrie de app.js (layerPieces, HOME, faceColor, P, R1, RT) :
 * chaque pièce est le polygone de la vue 2D extrudé verticalement.
 * Repère : x vers la droite, y vers le haut, z vers l'observateur
 * (z = coordonnée y de la vue 2D de dessus). Le twist fait tourner la moitié
 * x > 0 autour de l'axe x.
 */

class Sq1View3D {
  constructor(container) {
    this.el = container;
    this.M = 30;          // demi-épaisseur de la couche médiane
    this.H = 100;         // demi-hauteur du puzzle (cube de côté 2·S = 200)

    const r = this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    r.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    container.appendChild(r.domElement);

    const scene = this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(32, 1, 10, 5000);
    scene.add(new THREE.AmbientLight(0xffffff, 0.62));
    const key = new THREE.DirectionalLight(0xffffff, 0.62);
    key.position.set(250, 500, 420);
    scene.add(key);
    const fill = new THREE.DirectionalLight(0xffffff, 0.3);
    fill.position.set(-400, -350, -250);
    scene.add(fill);

    this.root = new THREE.Group();
    scene.add(this.root);

    this.mats = new Map();
    this.labelMats = new Map();
    this.key = null;
    this.resetCamera();
    this.bindControls();

    this.resize();
    if (window.ResizeObserver) new ResizeObserver(() => this.resize()).observe(container);
    else window.addEventListener('resize', () => this.resize());
  }

  // ------------------------------------------------------------ caméra
  resetCamera() {
    this.yaw = 0.55;      // vue de trois quarts : face avant + moitié droite
    this.pitch = 0.5;
    this.dist = 820;
    this.updateCamera();
  }
  updateCamera() {
    const c = Math.cos(this.pitch);
    this.camera.position.set(this.dist * c * Math.sin(this.yaw), this.dist * Math.sin(this.pitch), this.dist * c * Math.cos(this.yaw));
    this.camera.lookAt(0, 0, 0);
    this.render();
  }
  bindControls() {
    const cv = this.renderer.domElement;
    let drag = null;
    cv.addEventListener('pointerdown', e => {
      drag = { x: e.clientX, y: e.clientY };
      cv.setPointerCapture(e.pointerId);
    });
    cv.addEventListener('pointermove', e => {
      if (!drag) return;
      this.yaw -= (e.clientX - drag.x) * 0.01;
      this.pitch = Math.max(-1.45, Math.min(1.45, this.pitch + (e.clientY - drag.y) * 0.01));
      drag = { x: e.clientX, y: e.clientY };
      this.updateCamera();
    });
    const end = () => { drag = null; };
    cv.addEventListener('pointerup', end);
    cv.addEventListener('pointercancel', end);
    cv.addEventListener('wheel', e => {
      e.preventDefault();
      this.dist = Math.max(450, Math.min(1800, this.dist * Math.exp(e.deltaY * 0.001)));
      this.updateCamera();
    }, { passive: false });
    cv.addEventListener('dblclick', () => this.resetCamera());
  }
  resize() {
    const w = this.el.clientWidth, h = this.el.clientHeight;
    if (!w || !h) return;
    this.renderer.setSize(w, h, false);
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.render();
  }
  render() {
    this.renderer.render(this.scene, this.camera);
  }

  // --------------------------------------------------------- matériaux
  mat(color) {
    if (!this.mats.has(color))
      this.mats.set(color, new THREE.MeshLambertMaterial({ color, side: THREE.DoubleSide }));
    return this.mats.get(color);
  }
  labelMat(ch) {
    if (!this.labelMats.has(ch)) {
      const cv = document.createElement('canvas');
      cv.width = cv.height = 128;
      const g = cv.getContext('2d');
      g.fillStyle = '#1c1f26';
      g.font = 'bold 84px ui-monospace, Menlo, Consolas, monospace';
      g.textAlign = 'center';
      g.textBaseline = 'middle';
      g.fillText(ch, 64, 68);
      const tex = new THREE.CanvasTexture(cv);
      this.labelMats.set(ch, new THREE.MeshBasicMaterial({ map: tex, transparent: true, depthWrite: false, side: THREE.DoubleSide }));
    }
    return this.labelMats.get(ch);
  }

  // -------------------------------------------------------- géométrie
  /** Polygone 2D (coordonnées de la vue de dessus) extrudé entre y0 et y1. */
  prism(pts, y0, y1, color) {
    const shape = new THREE.Shape(pts.map(([x, y]) => new THREE.Vector2(x, -y)));
    const g = new THREE.ExtrudeGeometry(shape, { depth: y1 - y0, bevelEnabled: false });
    g.rotateX(-Math.PI / 2);
    g.translate(0, y0, 0);
    return new THREE.Mesh(g, this.mat(color));
  }
  /** Autocollant horizontal (face haute ou basse), légèrement rétréci. */
  capSticker(pts, y, color) {
    const cx = pts.reduce((s, q) => s + q[0], 0) / pts.length;
    const cy = pts.reduce((s, q) => s + q[1], 0) / pts.length;
    const k = 0.86;
    const shape = new THREE.Shape(pts.map(([x, yy]) => new THREE.Vector2(cx + (x - cx) * k, -(cy + (yy - cy) * k))));
    const g = new THREE.ShapeGeometry(shape);
    g.rotateX(-Math.PI / 2);
    g.translate(0, y, 0);
    return new THREE.Mesh(g, this.mat(color));
  }
  /** Autocollant vertical sur le segment extérieur a→b, entre y0 et y1. */
  sideSticker(a, b, y0, y1, color) {
    const m = 5, dx = b[0] - a[0], dz = b[1] - a[1], len = Math.hypot(dx, dz);
    const ux = dx / len, uz = dz / len;
    let nx = uz, nz = -ux;                                   // normale du segment
    if (nx * (a[0] + b[0]) + nz * (a[1] + b[1]) < 0) { nx = -nx; nz = -nz; }   // vers l'extérieur
    const o = 0.7;
    const p0 = [a[0] + ux * m + nx * o, a[1] + uz * m + nz * o];
    const p1 = [b[0] - ux * m + nx * o, b[1] - uz * m + nz * o];
    const lo = y0 + m, hi = y1 - m;
    const v = new Float32Array([
      p0[0], lo, p0[1], p1[0], lo, p1[1], p1[0], hi, p1[1],
      p0[0], lo, p0[1], p1[0], hi, p1[1], p0[0], hi, p0[1],
    ]);
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(v, 3));
    g.computeVertexNormals();
    return new THREE.Mesh(g, this.mat(color));
  }
  label(ch, at, y, bottom) {
    const g = new THREE.PlaneGeometry(30, 30);
    g.rotateX(bottom ? Math.PI / 2 : -Math.PI / 2);
    g.translate(at[0], y, at[1]);
    return new THREE.Mesh(g, this.labelMat(ch));
  }

  piece(p, bottom) {
    const { M, H } = this;
    const y0 = bottom ? -H : M + 0.5, y1 = bottom ? -M - 0.5 : H;
    const home = HOME[p.c];
    const body = home.bottom ? BODY_BOT : BODY_TOP;
    const th = p.angle;
    const grp = new THREE.Group();
    let pts, sides;
    if (!p.corner) {
      const a = P(th - 15, R1), b = P(th + 15, R1);
      pts = [[0, 0], a, b];
      sides = [[a, b, faceColor(home.angle)]];
    } else {
      const flipped = home.bottom !== bottom;
      const a = P(th - 30, R1), v = P(th, RT), b = P(th + 30, R1);
      pts = [[0, 0], a, v, b];
      sides = [[a, v, faceColor(home.angle + (flipped ? 45 : -45))],
               [v, b, faceColor(home.angle + (flipped ? -45 : 45))]];
    }
    grp.add(this.prism(pts, y0, y1, '#17181c'));
    const capY = bottom ? y0 - 0.7 : y1 + 0.7;
    grp.add(this.capSticker(pts, capY, body));
    for (const [a, b, col] of sides) grp.add(this.sideSticker(a, b, y0, y1, col));
    grp.add(this.label(p.c, P(th, p.corner ? RT * 0.55 : R1 * 0.62), bottom ? capY - 0.6 : capY + 0.6, bottom));
    return grp;
  }

  middle(right, ml) {
    const M = this.M - 0.5;
    const grp = new THREE.Group();
    const pts = right
      ? [P(0, R1), P(30, RT), P(120, RT), P(180, R1)]
      : [P(180, R1), P(210, RT), P(300, RT), P(360, R1)];
    if (right && ml === '?') {
      const m = this.prism(pts, -M, M, '#888888');
      m.material = new THREE.MeshLambertMaterial({ color: '#888888', transparent: true, opacity: 0.35 });
      grp.add(m);
      return grp;
    }
    grp.add(this.prism(pts, -M, M, '#17181c'));
    const cols = right ? [345, 75, 165] : [165, 255, 345];
    for (let i = 0; i < 3; i++) grp.add(this.sideSticker(pts[i], pts[i + 1], -M, M, faceColor(cols[i])));
    return grp;
  }

  // ----------------------------------------------------------- scène
  build(state) {
    this.root.traverse(o => { if (o.geometry) o.geometry.dispose(); });
    this.root.clear();
    const mk = () => new THREE.Group();
    this.topG = mk(); this.botG = mk();
    this.topR = mk(); this.botR = mk(); this.midR = mk();
    const midL = this.middle(false, state[24]);
    this.midR.add(this.middle(true, state[24]));
    for (const bottom of [false, true]) {
      const layer = bottom ? this.botG : this.topG;
      const right = bottom ? this.botR : this.topR;
      layer.add(right);
      for (const p of layerPieces(state, bottom)) {
        const m = this.piece(p, bottom);
        (Math.sin(rad(p.angle)) > 1e-6 ? right : layer).add(m);
      }
    }
    this.root.add(this.topG, this.botG, midL, this.midR);
    this.key = state;
  }

  /**
   * Dessine `state`. Options : top / bot = rotation en cours des couches
   * (degrés, sens horaire vu de dessus, comme la vue 2D) ; twist = angle (rad)
   * de la moitié droite pendant un twist.
   */
  draw(state, o = {}) {
    if (state !== this.key) this.build(state);
    const tw = o.twist || 0;
    this.topG.rotation.y = -rad(o.top || 0);
    this.botG.rotation.y = -rad(o.bot || 0);
    this.topR.rotation.x = tw;
    this.botR.rotation.x = tw;
    this.midR.rotation.x = (state[24] === '/' ? Math.PI : 0) + tw;
    this.render();
  }
}
