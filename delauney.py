#!/usr/bin/env python3

import sys, csv, json
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print('usage:', sys.argv[0], 'points.csv [voronoi.json]')
    sys.exit(1)

with open(sys.argv[1]) as f:
    points = np.array([
        [ float(x) for x in line ]
        for line in csv.reader(f, delimiter=',', skipinitialspace=True)
    ])
print(f'points: {len(points)}')

from scipy.spatial import Delaunay
delaunay = Delaunay(points)
print(f'triangles: {len(delaunay.simplices)}')

with open('triangles.txt', 'w') as f:
    f.write('# Points\n')
    for x, y in delaunay.points:
        f.write(f'{x} {y}\n')

    f.write('\n# Triangles\n')
    for a, b, c in delaunay.simplices:
        f.write(f'{a} {b} {c}\n')

plt.triplot(*points.T, delaunay.simplices,
            color='tab:blue', marker='o', markerfacecolor='tab:green', markeredgecolor='none',
            markersize=7)

plt.gca().set_aspect('equal', adjustable='box')
plt.tight_layout()
plt.savefig('1-triangulation.svg', bbox_inches='tight')

if len(sys.argv) < 3:
    sys.exit(0)

with open(sys.argv[2]) as f:
    voronoi = json.load(f)
    voronoi_vertices = np.array(voronoi['vertices'])
    voronoi_edges = np.array(voronoi['edges'])

for (x1,y1),(x2,y2) in voronoi_edges:
    plt.plot([x1,x2], [y1,y2], color='tab:orange')

plt.plot(*voronoi_vertices.T,
         color='none', marker='o', markerfacecolor='tab:red', markeredgecolor='none',
         markersize=5)

plt.gca().set_aspect('equal', adjustable='box')
plt.tight_layout()
plt.savefig('2-voronoi.svg', bbox_inches='tight')
