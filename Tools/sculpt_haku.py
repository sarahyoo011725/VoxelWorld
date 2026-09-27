"""
Sculpts Haku after his dragon form in Spirited Away: a slim cream body with lavender
belly plates, a feathery green mane from the crown to a tufted tail, a slender
wolfish head with big green eyes, pointed ears pink inside, two long ivory horns,
long silver whiskers, and thin mauve scaly legs with talons. Smooth shapes at half a
skin pixel per voxel, so only the finest steps show. Writes Resources/Models/haku/*.vox
with .origin sidecars (where the grid's corner sits relative to the part's pivot, in
voxels). Run from the repository root; needs numpy.

Units are voxels. Axes: x to the dragon's left, y up, z forward.
"""
import math
import os
import random
import struct

import numpy as np

OUT = 'Resources/Models/haku/'
rng = random.Random(11)

CREAM, CREAM_SHADE, CREAM_LIGHT = 1, 2, 3
BELLY, BELLY_LINE = 4, 5
MANE = [6, 7, 8, 9, 10]  # root to tip
HORN, HORN_RING = 11, 12
EAR_PINK = 13
IRIS, PUPIL, GLINT, LID = 14, 15, 16, 17
NOSE = 18
LEG, LEG_SCALE, CLAW = 19, 20, 21
WHISKER = 22
MOUTH = 23
IRIS_LIGHT = 24
PALETTE = {
    CREAM: (242, 239, 230), CREAM_SHADE: (224, 220, 210), CREAM_LIGHT: (251, 250, 245),
    BELLY: (184, 174, 200), BELLY_LINE: (146, 136, 166),
    6: (40, 112, 88), 7: (62, 142, 110), 8: (88, 170, 132), 9: (118, 196, 154), 10: (160, 222, 184),
    HORN: (238, 218, 166), HORN_RING: (208, 182, 124),
    EAR_PINK: (238, 168, 176),
    IRIS: (52, 160, 104), PUPIL: (16, 30, 26), GLINT: (255, 255, 255), LID: (46, 58, 62),
    NOSE: (112, 146, 148),
    LEG: (198, 160, 168), LEG_SCALE: (166, 126, 138), CLAW: (242, 234, 222),
    WHISKER: (230, 234, 238),
    MOUTH: (164, 150, 152),
    IRIS_LIGHT: (104, 200, 140),
}

LINK = 16  # voxels from one body link's pivot to the next (8 skin pixels)
RADIUS = [5.6, 6.2, 6.8, 7.1, 7.2, 7.0, 6.7, 6.3, 5.9, 5.4, 4.9, 4.3, 3.7, 3.1, 2.5, 2.0, 1.5]


class Grid:
    def __init__(self, lo, hi):
        self.lo = np.array(lo, dtype=int)
        shape = tuple(np.array(hi, dtype=int) - self.lo)
        self.cells = np.zeros(shape, dtype=np.uint8)
        ax = [np.arange(self.lo[i], self.lo[i] + shape[i]) + 0.5 for i in range(3)]
        self.x, self.y, self.z = np.meshgrid(ax[0], ax[1], ax[2], indexing='ij')

    def fill(self, mask, colour):
        self.cells[mask] = colour

    def paint(self, mask, colour):
        self.cells[mask & (self.cells > 0)] = colour

    def dab(self, p, radius, colour, over=True):
        c = np.array(p, float) - self.lo
        r = int(math.ceil(radius))
        for dx in range(-r, r + 1):
            for dy in range(-r, r + 1):
                for dz in range(-r, r + 1):
                    cx, cy, cz = int(math.floor(c[0])) + dx, int(math.floor(c[1])) + dy, int(math.floor(c[2])) + dz
                    if not (0 <= cx < self.cells.shape[0] and 0 <= cy < self.cells.shape[1] and 0 <= cz < self.cells.shape[2]):
                        continue
                    if (cx + 0.5 - c[0]) ** 2 + (cy + 0.5 - c[1]) ** 2 + (cz + 0.5 - c[2]) ** 2 <= radius * radius:
                        if over or self.cells[cx, cy, cz] == 0:
                            self.cells[cx, cy, cz] = colour

    def strand(self, points, r0, r1, colours, over=True):
        """a tapering rod along a polyline, coloured root to tip"""
        pts = [np.array(p, float) for p in points]
        lengths = [np.linalg.norm(b - a) for a, b in zip(pts, pts[1:])]
        total = sum(lengths) or 1.0
        run = 0.0
        for a, b, length in zip(pts, pts[1:], lengths):
            steps = max(1, int(length * 2))
            for s in range(steps + 1):
                t = (run + length * s / steps) / total
                colour = colours[min(len(colours) - 1, int(t * len(colours)))] if isinstance(colours, list) else colours
                self.dab(a + (b - a) * s / steps, r0 + (r1 - r0) * t, colour, over)
            run += length

    def save(self, name):
        os.makedirs(OUT, exist_ok=True)
        xs, ys, zs = np.nonzero(self.cells)
        assert max(self.cells.shape) <= 256, (name, self.cells.shape)
        voxels = bytearray()
        for x, y, z in zip(xs, ys, zs):
            # MagicaVoxel stands z up and y forward
            voxels += struct.pack('<4B', x, z, y, self.cells[x, y, z])
        size = struct.pack('<3i', self.cells.shape[0], self.cells.shape[2], self.cells.shape[1])
        rgba = b''
        for i in range(1, 256):
            r, g, b = PALETTE.get(i, (255, 0, 255))
            rgba += struct.pack('<4B', r, g, b, 255)
        rgba += struct.pack('<4B', 0, 0, 0, 0)

        def chunk(tag, content):
            return tag + struct.pack('<2i', len(content), 0) + content
        children = chunk(b'SIZE', size) + chunk(b'XYZI', struct.pack('<i', len(xs)) + bytes(voxels)) + chunk(b'RGBA', rgba)
        with open(OUT + name + '.vox', 'wb') as f:
            f.write(b'VOX ' + struct.pack('<i', 150) + b'MAIN' + struct.pack('<2i', 0, len(children)) + children)
        with open(OUT + name + '.vox.origin', 'w') as f:
            f.write('%d %d %d\n' % tuple(self.lo))
        return len(xs)


def ellipsoid(g, c, r):
    return ((g.x - c[0]) / r[0]) ** 2 + ((g.y - c[1]) / r[1]) ** 2 + ((g.z - c[2]) / r[2]) ** 2 <= 1.0


def capsule(g, a, b, ra, rb, squash_y=1.0):
    a, b = np.array(a, float), np.array(b, float)
    ab = b - a
    px, py, pz = g.x - a[0], g.y - a[1], g.z - a[2]
    t = np.clip((px * ab[0] + py * ab[1] + pz * ab[2]) / float(ab @ ab), 0, 1)
    dx, dy, dz = px - ab[0] * t, (py - ab[1] * t) / squash_y, pz - ab[2] * t
    return dx * dx + dy * dy + dz * dz <= (ra + (rb - ra) * t) ** 2


def mane_tuft(g, root, length, lean, spread=1.0, thick=1.3):
    """a wisp of mane: up from the root, sweeping back and curling at the tip"""
    x0, y0, z0 = root
    side = rng.uniform(-spread, spread)
    pts = []
    for k in range(5):
        t = k / 4.0
        pts.append((x0 + side * t * 3, y0 + length * (0.9 * t - 0.25 * t * t), z0 - length * lean * t * t - length * 0.3 * t))
    g.strand(pts, thick, 0.45, MANE, over=False)


def shade_body(g, cx, cy, ry, mask):
    """cream with a light top and a shaded flank, belly plates underneath"""
    rel = (g.y - cy) / ry
    g.fill(mask, CREAM)
    g.paint(mask & (rel > 0.55), CREAM_LIGHT)
    g.paint(mask & (rel < -0.1) & (rel >= -0.45), CREAM_SHADE)
    belly = mask & (rel < -0.45)
    g.paint(belly, BELLY)
    g.paint(belly & (np.floor(g.z) % 5 == 0), BELLY_LINE)


def link(i):
    r0, r1 = RADIUS[i], RADIUS[i + 1]
    ry0, ry1 = r0 * 1.1, r1 * 1.1
    g = Grid((-12, -12, -LINK - 32), (13, 32, 12))
    body = capsule(g, (0, 0, 0), (0, 0, -LINK), r0, r1, 1.1) | ellipsoid(g, (0, 0, 0), (r0, ry0, r0))
    shade_body(g, 0, 0, (ry0 + ry1) / 2, body)
    #the mane: long and full on the neck, a low crest down the back, thinning to the tail
    length = [22, 20, 16, 12, 10, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4][i]
    for z in range(0, -LINK, -3):
        t = -z / LINK
        top = ry0 + (ry1 - ry0) * t
        for k in range(2 if i < 4 else 1):
            mane_tuft(g, (rng.uniform(-1.2, 1.2), top - 1.2, z - rng.uniform(0, 1.5)), length * rng.uniform(0.7, 1.1), 0.55, 1.0, 1.1)
    return g


def eye(g, side, ey, ez, ery, erz):
    """one eye on each side, painted on the outermost voxel so there can only be two"""
    sx = g.cells.shape[0]
    for yi in range(g.cells.shape[1]):
        for zi in range(g.cells.shape[2]):
            y, z = g.lo[1] + yi + 0.5, g.lo[2] + zi + 0.5
            d = ((y - ey) / ery) ** 2 + ((z - ez) / erz) ** 2
            if d > 1.45:
                continue
            xs = range(sx - 1, -1, -1) if side > 0 else range(sx)
            outer = next((xi for xi in xs if g.cells[xi, yi, zi] != 0), None)
            if outer is None or abs(g.lo[0] + outer + 0.5) < 2.8:
                continue
            if d > 1.0:
                colour = LID if y > ey else None
            elif d > 0.72:
                colour = LID
            elif ((y - ey) / ery) ** 2 + ((z - ez) / (erz * 0.7)) ** 2 < 0.22:
                colour = PUPIL
            else:
                colour = IRIS_LIGHT if y < ey - 0.3 * ery else IRIS
            if abs(y - (ey + ery * 0.35)) < 0.6 and abs(z - (ez + erz * 0.25)) < 0.6:
                colour = GLINT
            if colour is None:
                continue
            for depth in range(2):
                xi = outer - depth * (1 if side > 0 else -1)
                if 0 <= xi < sx and g.cells[xi, yi, zi] != 0:
                    g.cells[xi, yi, zi] = colour


def head():
    g = Grid((-40, -26, -80), (41, 48, 34))
    #cranium, brow, round cheeks, a long slender muzzle and the jaw under it
    skull = ellipsoid(g, (0, 2, 5), (5.8, 5.6, 7.5))
    skull |= ellipsoid(g, (0, 4.2, 9), (4.4, 3.4, 5.5))
    skull |= ellipsoid(g, (3.2, 0, 7), (2.6, 3.2, 4.2)) | ellipsoid(g, (-3.2, 0, 7), (2.6, 3.2, 4.2))
    skull |= capsule(g, (0, 1.6, 10), (0, -0.4, 24), 3.9, 2.2, 0.85)
    jaw = capsule(g, (0, -2.4, 8), (0, -3.2, 21), 3.0, 1.5, 0.8)
    skull |= jaw | ellipsoid(g, (0, -1, 0), (5.4, 5.6, 5))
    g.fill(skull, CREAM)
    g.paint(skull & (g.y > 4.5), CREAM_LIGHT)
    g.paint(skull & (g.y < -1.5), CREAM_SHADE)
    g.paint(skull & (g.y < -3.5) & (g.z < 6), BELLY)
    #the nose pad and a mouth line along each side only
    g.fill(ellipsoid(g, (0, 0.2, 24.2), (1.9, 1.4, 1.4)), NOSE)
    g.paint((np.abs(g.y + 1.6) < 0.5) & (np.abs(g.x) > 1.2) & (g.z > 11) & (g.z < 21), MOUTH)
    eye(g, 1, 3.3, 10.6, 2.3, 3.0)
    eye(g, -1, 3.3, 10.6, 2.3, 3.0)
    #pointed ears standing out and back, pink inside
    for s in (1, -1):
        g.strand([(4.2 * s, 5.5, 5), (7.5 * s, 9.5, 2.5), (10 * s, 13, 0)], 2.4, 0.4, CREAM)
        g.strand([(5.6 * s, 7.0, 5.0), (7.6 * s, 9.8, 3.4), (9.4 * s, 12.2, 1.4)], 1.2, 0.3, EAR_PINK)
    #two long ivory horns sweeping up and back from the brow
    for s in (1, -1):
        pts = []
        for k in range(9):
            t = k / 8.0
            pts.append((s * (2.0 + 4.0 * t), 6 + 19 * t, 5 - 5 * t - 5 * t * t))
        g.strand(pts, 1.45, 0.45, [HORN, HORN, HORN_RING, HORN, HORN, HORN_RING, HORN, HORN])
    #the forelock and mane: full from the crown down the back of the neck
    for k in range(24):
        x = rng.uniform(-2.6, 2.6)
        z = rng.uniform(-4, 3)
        top = 5.6 * math.sqrt(max(0.0, 1 - (x / 5.8) ** 2 - ((z - 5) / 7.5) ** 2)) + 2
        mane_tuft(g, (x, top - 1, z), rng.uniform(11, 18), 0.7, 1.4, 1.3)
    #long silver whiskers from the muzzle, streaming out and back in a loose wave
    for s in (1, -1):
        pts = []
        for k in range(25):
            t = k / 24.0
            pts.append((s * (2.4 + 26 * t - 10 * t * t), -0.5 + 10 * math.sin(t * math.pi * 1.4) * t, 20 - 92 * t))
        g.strand(pts, 0.6, 0.45, WHISKER)
    return g


def leg():
    g = Grid((-8, -26, -8), (9, 5, 12))
    #a cream thigh blending into the body, a slender mauve scaly shank, three talons forward and one back
    g.fill(ellipsoid(g, (0, -2, 0), (2.8, 5, 3.2)), CREAM)
    shank = capsule(g, (0, -6, 0.5), (0, -14, -1.5), 1.6, 1.2) | capsule(g, (0, -14, -1.5), (0, -20, 0), 1.2, 1.0)
    g.fill(shank & (g.cells == 0), LEG)
    g.paint(shank & (np.floor(g.y) % 2 == 0) & (g.cells == LEG), LEG_SCALE)
    for s, reach in ((-1.6, 5.5), (0, 6.5), (1.6, 5.5)):
        g.strand([(0, -20, 0.5), (s, -21.2, reach * 0.6), (s * 1.3, -21.8, reach)], 0.8, 0.55, LEG)
        g.strand([(s * 1.3, -21.8, reach), (s * 1.4, -22.8, reach + 1.2)], 0.5, 0.3, CLAW)
    g.strand([(0, -20, -0.5), (0, -21.5, -3.5)], 0.7, 0.4, LEG)
    g.dab((0, -22.2, -4.2), 0.5, CLAW)
    return g


def tail():
    g = Grid((-16, -8, -40), (17, 40, 6))
    r = RADIUS[-1]
    tip = capsule(g, (0, 0, 0), (0, 2, -12), r, 0.8, 1.1) | ellipsoid(g, (0, 0, 0), (r, r * 1.1, r))
    shade_body(g, 0, 0, r * 1.1, tip)
    #a feathery green tuft fanning up from the tip
    for k in range(26):
        a = rng.uniform(-0.5, 0.5)
        root = (rng.uniform(-0.8, 0.8), 1.5, rng.uniform(-12, -4))
        length = rng.uniform(14, 26)
        pts = [root]
        for j in range(1, 5):
            t = j / 4.0
            pts.append((root[0] + math.sin(a) * length * t, root[1] + length * t * 0.95, root[2] - length * (0.35 * t + 0.25 * t * t)))
        g.strand(pts, 1.5, 0.45, MANE, over=False)
    return g


if __name__ == '__main__':
    total = 0
    for i in range(16):
        total += link(i).save('body_%02d' % i)
    total += head().save('head')
    total += leg().save('leg')
    total += tail().save('tail')
    print('%d voxels' % total)
