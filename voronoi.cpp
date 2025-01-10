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
struct ordered_pair: std::array<T, 2> {
    using base_t = std::array<T, 2>;
    ordered_pair(T a, T b) {
        if (a < b) {
            static_cast<base_t&>(*this) = { a, b };
        } else {
            static_cast<base_t&>(*this) = { b, a };
        }
    }
};

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
      std::array<int, 2> // voronoi vertices:
      // (index of voronoi vertex, corresponding third triangle vertex) × 2
    > edges;

    // Voronoi vertices: centers of circumscribed circles of triangles
    std::vector<point_t> vertices(triangles.size());

    cout << "{\n\"vertices\":[\n";
    bool first = true;

    for (size_t t = 0; t < triangles.size(); ++t) {
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

        if (first) {
            first = false;
        } else {
            cout << ",\n";
        }
        cout << '[' << cx << ',' << cy << ']';
        vertices[t] = { cx, cy };

        for (int permutation = 3;;) {
            auto& vv = edges[{ i, j }]; // edges: triangle edge -> Voronoi edge
            if (vv[1] == 0) {
              vv = { int(t), -(k+1) };
              // negative second vertex represents the third triangle vertex
              // until the second voronoi vertex is set
            } else {
              vv[1] = int(t);
            }

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
        const point_t v1 = vertices[vv[0]]; // first Voronoi vertex
        point_t v2; // second Voronoi vertex
        if (vv[1] < 0) { // if no Voronoi vertex across the edge, use edge center
            const auto [ x1, y1 ] = points[edge[0]];
            const auto [ x2, y2 ] = points[edge[1]];

            // ax + by + c = 0
            const double a = y1 - y2;
            const double b = x2 - x1;
            const double c = x1*y2 - y1*x2;

            const point_t t3 = points[-(vv[1]+1)]; // the other vertex of the triangle

            // if the single voronoi vertex is on the oposite side
            // from the third triangle vertex
            if ( ( a*v1[0] + b*v1[1] + c < 0 )
              != ( a*t3[0] + b*t3[1] + c < 0 )
            ) continue;

            v2 = { (x1 + x2) * 0.5, (y1 + y2) * 0.5 };
        } else {
          v2 = vertices[vv[1]];
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
