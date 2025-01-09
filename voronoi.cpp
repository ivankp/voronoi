#include <string>
#include <vector>
#include <array>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

using std::cout, std::cerr, std::endl;

enum {
    READ_NULL,
    READ_POINTS,
    READ_TRIANGLES
} read_section = READ_NULL;

auto sq(auto x) { return x*x; }

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " file\n";
        return 1;
    }

    using point_t = std::array<double, 2>;
    std::vector<point_t> points;
    std::vector<std::array<unsigned, 3>> triangles;

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

    struct edge {
        unsigned a, b;
        edge(unsigned _a, unsigned _b) {
            if (_a < _b) {
                a = _a;
                b = _b;
            } else {
                a = _b;
                b = _a;
            }
        }
        auto operator<=>(const edge&) const = default;
    };
    // Voronoi diagram edges
    std::map<edge, std::array<std::array<const point_t*, 2>, 2>> edges;

    // Centers of circumscribed circles of triangles
    std::vector<point_t> vertices(triangles.size());

    cout << "{\n\"vertices\":[\n";
    bool first = true;

    for (size_t t = 0; t < triangles.size(); ++t) {
        auto [ i, j, k ] = triangles[t]; // triangle vertices
        double d = 0, py = 1, cx = 0, cy = 0;
        for (unsigned permutation = 3;;) {
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

        if (first) {
            first = false;
        } else {
            cout << ",\n";
        }
        cout << '[' << cx << ',' << cy << ']';
        const point_t& p = vertices[t] = { cx, cy };

        for (unsigned permutation = 3;;) {
            auto& vv = edges[{ i, j }]; // edges: triangle edge -> Voronoi edge
            vv[!!vv[0][0]] = { &p, &points[k] };

            if (!--permutation)
                break;

            const auto tmp = i;
            i = j;
            j = k;
            k = tmp;
        }
    }

    cout << "\n],\n\"edges\":[\n";
    first = true;
    for (auto& [ edge, vv ] : edges) {
        const point_t v1 = *vv[0][0];
        point_t v2;
        if (!vv[1][0]) { // if no Voronoi vertex across the edge, use edge center
            const auto [ xa, ya ] = points[edge.a];
            const auto [ xb, yb ] = points[edge.b];
            v2 = { (xa + xb) * 0.5, (ya + yb) * 0.5 };

            /*
            // ax + by + c = 0
            const double a = ya - yb;
            const double b = xb - xa;
            const double c = xa*yb - ya*xb;

            const point_t v3 = *vv[0][1]; // the other triangle vertex

            // if on opposite sides of the triangle edge
            if ( ( a*v2[0] + b*v2[1] + c < 0 )
              != ( a*v3[0] + b*v3[1] + c < 0 )
            ) continue;
            */
        } else {
          v2 = *vv[1][0];
        }
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
