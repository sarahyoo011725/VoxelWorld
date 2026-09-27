"""
Shared tools for sculpting mob parts as MagicaVoxel models: a voxel grid with shape
masks, dabs and tapering strands, painted eyes, and a .vox writer that also leaves a
.origin sidecar (where the grid's corner sits relative to the part's pivot, in voxels).

Units are voxels. Axes: x to the mob's left, y up, z forward.
"""
import math
import os
import struct

import numpy as np


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

    def save(self, out, name, palette):
        os.makedirs(out, exist_ok=True)
        xs, ys, zs = np.nonzero(self.cells)
        assert max(self.cells.shape) <= 256, (name, self.cells.shape)
        voxels = bytearray()
        for x, y, z in zip(xs, ys, zs):
            # MagicaVoxel stands z up and y forward
            voxels += struct.pack('<4B', x, z, y, self.cells[x, y, z])
        size = struct.pack('<3i', self.cells.shape[0], self.cells.shape[2], self.cells.shape[1])
        rgba = b''
        for i in range(1, 256):
            r, g, b = palette.get(i, (255, 0, 255))
            rgba += struct.pack('<4B', r, g, b, 255)
        rgba += struct.pack('<4B', 0, 0, 0, 0)

        def chunk(tag, content):
            return tag + struct.pack('<2i', len(content), 0) + content
        children = chunk(b'SIZE', size) + chunk(b'XYZI', struct.pack('<i', len(xs)) + bytes(voxels)) + chunk(b'RGBA', rgba)
        with open(out + name + '.vox', 'wb') as f:
            f.write(b'VOX ' + struct.pack('<i', 150) + b'MAIN' + struct.pack('<2i', 0, len(children)) + children)
        with open(out + name + '.vox.origin', 'w') as f:
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


def eye(g, side, ey, ez, ery, erz, colours, min_x=2.8):
    """one eye on each side, painted on the outermost voxel so there can only be two;
    colours are (lid, pupil, iris, lower iris, glint)"""
    LID, PUPIL, IRIS, IRIS_LIGHT, GLINT = colours
    sx = g.cells.shape[0]
    for yi in range(g.cells.shape[1]):
        for zi in range(g.cells.shape[2]):
            y, z = g.lo[1] + yi + 0.5, g.lo[2] + zi + 0.5
            d = ((y - ey) / ery) ** 2 + ((z - ez) / erz) ** 2
            if d > 1.45:
                continue
            xs = range(sx - 1, -1, -1) if side > 0 else range(sx)
            outer = next((xi for xi in xs if g.cells[xi, yi, zi] != 0), None)
            if outer is None or abs(g.lo[0] + outer + 0.5) < min_x:
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


def mirrored(g):
    """the same part for the other side: x flipped about the pivot"""
    m = Grid((-(g.lo[0] + g.cells.shape[0]), g.lo[1], g.lo[2]), (-g.lo[0], g.lo[1] + g.cells.shape[1], g.lo[2] + g.cells.shape[2]))
    m.cells = g.cells[::-1].copy()
    return m
