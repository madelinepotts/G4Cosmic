#pragma once
#include <array>
#include <cstddef>
namespace WarpTrackGeometry {

struct Vertex { double u; double z; };

struct Piece { int layer; int bar; int channelOffset; int centerIndex; bool sensitive; std::array<Vertex, 3> v; };

struct HodoscopeInstance { int id; double rackU; };

struct ServerType { const char* name; int heightU; double widthMM; double depthMM; const char* chassisMaterial; double wallThicknessMM; double interiorDensityGCM3; };

struct ServerInstance { int id; double rackU; int typeIndex; };

inline constexpr double rackUnitMM = 44.450000000000003;
inline constexpr double widthMM = 482.60000000000002;
inline constexpr double depthMM = 904.875;
inline constexpr double hodoscopeHeightMM = 88.900000000000006;
inline constexpr int channelsPerHodoscope = 25;

inline constexpr bool serverModelEnabled = true;

inline constexpr std::array<HodoscopeInstance, 3> hodoscopes = {
  HodoscopeInstance{0, 4},
  HodoscopeInstance{1, 14},
  HodoscopeInstance{2, 27},
};

inline constexpr std::array<ServerType, 2> serverTypes = {
  ServerType{"generic_1u", 1, 482.60000000000002, 800, "G4_Al", 1.5, 0.29999999999999999},
  ServerType{"generic_2u", 2, 482.60000000000002, 800, "G4_STAINLESS-STEEL", 1.5, 0.29999999999999999},
};

inline constexpr std::array<ServerInstance, 5> servers = {
  ServerInstance{0, 7, 0},
  ServerInstance{1, 9, 1},
  ServerInstance{2, 17, 1},
  ServerInstance{3, 20, 0},
  ServerInstance{4, 23, 1},
};

inline constexpr std::array<Piece, 25> pieces = {{
  Piece{0, 0, 0, 0, true, {Vertex{-0.5, -0.5}, Vertex{-0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 1, 0, 0, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 2, 0, 1, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 3, 0, 2, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 4, 0, 3, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 5, 0, 4, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 6, 0, 5, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 7, 0, 6, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 8, 0, 7, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 9, 0, 8, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 10, 0, 9, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 11, 0, 10, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 12, 0, 11, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 13, 0, 12, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{0, 14, 0, 13, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{0, 15, 0, 13, true, {Vertex{0, 0.5}, Vertex{0.5, 0.5}, Vertex{0.5, -0.5}}},
  Piece{1, 0, 16, 0, true, {Vertex{-0.5, -0.5}, Vertex{-0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{1, 1, 16, 0, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{1, 2, 16, 1, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{1, 3, 16, 2, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{1, 4, 16, 3, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{1, 5, 16, 4, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{1, 6, 16, 5, true, {Vertex{-0.5, -0.5}, Vertex{0.5, -0.5}, Vertex{0, 0.5}}},
  Piece{1, 7, 16, 6, true, {Vertex{-0.5, 0.5}, Vertex{0.5, 0.5}, Vertex{0, -0.5}}},
  Piece{1, 8, 16, 6, true, {Vertex{0, -0.5}, Vertex{0.5, -0.5}, Vertex{0.5, 0.5}}},
}};

}  // namespace WarpTrackGeometry
