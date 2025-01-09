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
    std::map<edge, std::array<const point_t*, 2>> edges;

    // Centers of circumscribed circles of triangles
    std::vector<point_t> vertices(triangles.size()); /////// <<<<<<<
    // TODO: reserve correct number of additional vertices
    vertices.reserve(triangles.size() * 2);
    // TODO: currently, vertices must not reallocate

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
            auto& vv = edges[{ i, j }]; // Voronoi vertex
            vv[!!vv[0]] = &p;

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
        if (!vv[1]) { // if no Voronoi vertex across the edge, use edge center
            const auto [ xa, ya ] = points[edge.a];
            const auto [ xb, yb ] = points[edge.b];
            vv[1] = &(vertices.emplace_back() = { (xa + xb) * 0.5, (ya + yb) * 0.5 });
        }
        const auto& a = *vv[0];
        const auto& b = *vv[1];
        if (first) {
            first = false;
        } else {
            cout << ",\n";
        }
        cout << "["
            "[" << a[0] << ',' << a[1] << "],"
            "[" << b[0] << ',' << b[1] << "]]";
    }
    cout << "\n]\n}\n";
}
