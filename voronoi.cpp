#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

using std::cout, std::cerr, std::endl;

enum {
    READ_NULL,
    READ_POINTS,
    READ_TRIANGLES
} read_section = READ_NULL;

template <typename T>
struct ordered_pair: std::array<T, 2> {
    using base_t = std::array<T, 2>;
    base_t& operator+() noexcept { return *this; }
    const base_t& operator+() const noexcept { return *this; }
    ordered_pair(T a, T b) {
        if (a < b) {
            +*this = { a, b };
        } else {
            +*this = { b, a };
        }
    }
};

template <typename... T>
constexpr auto sq(T... x) noexcept
requires (std::is_arithmetic_v<T> && ...)
{ return ((x * x) + ...); }

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " file\n";
        return 1;
    }

    using point_t = std::array<double, 2>;
    std::vector<point_t> points;
    std::vector<std::array<int, 3>> triangles;

    {
        std::ifstream file(argv[1]);
        if (!file) {
            cerr << "Unable to open file: " << argv[1] << '\n';
            return 1;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;
            if (line[0] == '#') {
                if (line == "# Points") {
                    read_section = READ_POINTS;
                } else if (line == "# Triangles") {
                    read_section = READ_TRIANGLES;
                } else {
                    cerr << "Unexpected section label: " << line << '\n';
                    return 1;
                }
                continue;
            }
            std::stringstream ss(line);
            switch (read_section) {
                case READ_POINTS:
                    for (auto& val : points.emplace_back())
                        ss >> val;
                    break;
                case READ_TRIANGLES:
                    for (auto& val : triangles.emplace_back())
                        ss >> val;
                    break;
                default:
                    cerr << "Missing section label\n";
                    return 1;
            }
        }
    }

    // Voronoi diagram edges
    std::map<
      ordered_pair<int>, // triangle edge: indices of triangle vertices
      std::array<int, 2> // voronoi vertices
    > edges;

    // Voronoi vertices: centers of circumscribed circles of triangles
    std::vector<point_t> vertices(triangles.size());

    for (int t = 0, nt = triangles.size(); t < nt; ++t) {
        auto [ i, j, k ] = triangles[t]; // triangle vertices
        double d = 0, py = 1, cx = 0, cy = 0;
        for (int permutation = 3;;) {
            const auto [ xi, yi ] = points[i];
            const auto [ xj, yj ] = points[j];
            const auto [ xk, yk ] = points[k];

            const double dy = yi - yj;
            const double dl = dy * xk;

            py *= dy;
            d += dl;
            cx += dl * xk;
            cy += (yi*yi + xi*xi) * (xk - xj);

            if (!--permutation)
                break;

            const auto tmp = i;
            i = j;
            j = k;
            k = tmp;
        }
        cx -= py;
        d *= 2;
        cx /= d;
        cy /= d;

        vertices[t] = { cx, cy };

        for (int permutation = 3;;) {
            auto& vv = edges[{ i, j }]; // edges: triangle edge -> Voronoi edge
            if (vv[1] == 0) {
              vv = { t, -(k+1) };
              // negative second vertex represents the third triangle vertex
              // until the second voronoi vertex is set
            } else {
              vv[1] = t;
            }
            // TODO: need to check for exact overlap of Voronoi vertices: if (vv[1] > 0) (0 could mean unassigned)

            if (!--permutation)
                break;

            const auto tmp = i;
            i = j;
            j = k;
            k = tmp;
        }
    }

    std::set<int> remove_vertices;
    std::vector<typename decltype(edges)::const_iterator> remove_edges;
    decltype(edges) edges2;

    for (auto it = edges.begin(); it != edges.end(); ) {
        auto& [ triangle_vertices, voronoi_vertices ] = *it;
        auto [ v1, v2 ] = voronoi_vertices;
        if (v2 < 0) { // if no Voronoi vertex across the edge, use edge center
            const auto [ t1, t2 ] = +triangle_vertices;
            const auto [ tx1, ty1 ] = points[t1];
            const auto [ tx2, ty2 ] = points[t2];

            // dy * x - dx * y + xy = 0
            const double tdx = tx1 - tx2;
            const double tdy = ty1 - ty2;
            const double txy = tx1 * ty2 - ty1 * tx2;

            const auto [ tx3, ty3 ] = points[-(v2+1)]; // the other vertex of the triangle
            const auto [ vx1, vy1 ] = vertices[v1];

            if ( ( tdx * vy1 - tdy * vx1 < txy )
              == ( tdx * ty3 - tdy * tx3 < txy )
            ) {
              // if the single voronoi vertex is on the same side
              // from the third triangle vertex,
              // use the edge center as the missing vertex
              vertices.push_back({ (tx1 + tx2) * 0.5, (ty1 + ty2) * 0.5 });
              voronoi_vertices[1] = int(vertices.size() - 1);
            } else {
              // if the voronoi vertex is on the opposite side,
              // truncate at the edge
              it = edges.erase(it);

              int t = -1;
              for (auto it = edges.begin(); it != edges.end(); ++it) {
                auto [ a, b ] = it->second;
                if (b < 0) {
                  continue;
                } else if (b == v1) {
                  v2 = a;
                } else if (a == v1) {
                  v2 = b;
                } else {
                  continue;
                }

                const auto [ vx2, vy2 ] = vertices[v2];

                const double vdx = vx1 - vx2;
                const double vdy = vy1 - vy2;
                const double vxy = vx1 * vy2 - vy1 * vx2;

                const double d = tdx * vdy - vdx * tdy;

                const double x = (txy * vdx - vxy * tdx) / d;
                const double y = (txy * vdy - vxy * tdy) / d;

                t = t < 0
                  ? ( sq(tx1 - x, ty1 - y) < sq(tx2 - x, ty2 - y) ) ? t1 : t2
                  : t == t2 ? t1 : t2;

                vertices.push_back({ x, y });
                edges2[{ -1, t }] = { v2, int(vertices.size() - 1) };

                remove_edges.push_back(it);
              }

              // TODO: remove v1 here
              remove_vertices.insert(v1);

              continue;
            }
        }
        ++it;
    }

    {
        std::ranges::sort(remove_edges, {},
            [](const auto& it) -> const auto& { return it->first; }
        );
        const auto [first, last] = std::ranges::unique(remove_edges);
        remove_edges.erase(first, last);
        for (auto it : remove_edges) {
          edges.erase(it);
        }
        edges.merge(std::move(edges2));
    }

    cout << "{\n\"vertices\":[\n";
    bool first = true;
    for (size_t i = 0; i < triangles.size(); ++i) {
        if (remove_vertices.contains(i))
          continue;
        if (first) {
            first = false;
        } else {
            cout << ",\n";
        }
        auto [ x, y ] = vertices[i];
        cout << '[' << x << ',' << y << ']';
    }

    cout << "\n],\n\"edges\":[\n";
    first = true;
    for (auto& [ tt, vv ] : edges) {
        const point_t v1 = vertices[vv[0]]; // first  Voronoi vertex

        // if (vv[1] < 0)
        //     continue;
        const point_t v2 = vertices[vv[1]]; // second Voronoi vertex

        if (first) {
            first = false;
        } else {
            cout << ",\n";
        }
        cout << "["
            "[" << v1[0] << ',' << v1[1] << "],"
            "[" << v2[0] << ',' << v2[1] << "]]";
    }
    cout << "\n]\n}\n";
}
