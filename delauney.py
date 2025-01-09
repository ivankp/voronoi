#!py -3

import csv
import numpy as np
import matplotlib.pyplot as plt

with open('points.csv') as f:
    nodes = np.array([
        [ float(x) for x in line ]
        for line in csv.reader(f, delimiter=',', skipinitialspace=True)
    ])
print(f'nodes: {len(nodes)}')

from scipy.spatial import Delaunay
triang = Delaunay(nodes)
print(f'triangles: {len(triang.simplices)}')

plt.triplot(*nodes.T, triang.simplices,
            color='tab:blue', marker='o', markerfacecolor='tab:green', markeredgecolor='none',
            markersize=7)

plt.gca().set_aspect('equal', adjustable='box')
plt.tight_layout()
plt.savefig('1-triangulation.svg', bbox_inches='tight')

voronoi_vertices = np.array([
])

voronoi_edges = [
]

for (x1,y1),(x2,y2) in voronoi_edges:
    plt.plot([x1,x2], [y1,y2], color='tab:orange')

plt.plot(*voronoi_vertices.T,
            color='none', marker='o', markerfacecolor='tab:red', markeredgecolor='none',
            markersize=5)

plt.gca().set_aspect('equal', adjustable='box')
plt.tight_layout()
plt.savefig('2-voronoi.svg', bbox_inches='tight')
