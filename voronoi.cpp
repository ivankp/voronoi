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

template <typename T>
struct ordered_pair {
    T a, b;
    ordered_pair(T _a, T _b) {
        if (_a < _b) {
            a = _a;
            b = _b;
        } else {
            a = _b;
            b = _a;
        }
    }
    auto operator<=>(const ordered_pair&) const = default;
};

struct index {
  unsigned i = -1;
  constexpr index() noexcept = default;
  constexpr index(unsigned i) noexcept: i(i) { }
  constexpr operator unsigned() const noexcept { return i; }
  constexpr bool empty() const noexcept { return i == unsigned(-1); }
};

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

    // Voronoi diagram edges
    std::map<
      ordered_pair<unsigned>, // triangle edge: indices of triangle vertices
      std::array<std::array<index, 2>, 2> // voronoi vertices:
      // (index of voronoi vertex, corresponding third triangle vertex) × 2
    > edges;

    // Voronoi vertices: centers of circumscribed circles of triangles
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
        vertices[t] = { cx, cy };

        for (unsigned permutation = 3;;) {
            auto& vv = edges[{ i, j }]; // edges: triangle edge -> Voronoi edge
            vv[!vv[0][0].empty()] = { t, k };

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
        const point_t v1 = vertices[vv[0][0]]; // first Voronoi vertex
        point_t v2; // second Voronoi vertex
        if (vv[1][0].empty()) { // if no Voronoi vertex across the edge, use edge center
            const auto [ x1, y1 ] = points[edge.a];
            const auto [ x2, y2 ] = points[edge.b];

            // ax + by + c = 0
            const double a = y1 - y2;
            const double b = x2 - x1;
            const double c = x1*y2 - y1*x2;

            const point_t t3 = points[vv[0][1]]; // the other vertex of the triangle

            // if the single voronoi vertex is on the oposite side
            // from the third triangle vertex
            if ( ( a*v1[0] + b*v1[1] + c < 0 )
              != ( a*t3[0] + b*t3[1] + c < 0 )
            ) continue;

            v2 = { (x1 + x2) * 0.5, (y1 + y2) * 0.5 };
        } else {
          v2 = vertices[vv[1][0]];
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
