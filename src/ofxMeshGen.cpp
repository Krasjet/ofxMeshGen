#include "ofxMeshGen.h"

void recalcNormals(ofMesh& mesh, bool CW_winding)
{
  if (mesh.getMode() != OF_PRIMITIVE_TRIANGLES) {
    throw std::invalid_argument("The mode of input mesh must be OF_PRIMITIVE_TRIANGLES.");
  }

  std::vector<glm::vec3> normals(mesh.getNumVertices());

  // for each triangle, add normal to vertex
  for (size_t i = 0; i < mesh.getNumIndices(); i += 3) {
    auto i1 = mesh.getIndex(i);
    auto i2 = mesh.getIndex(i + 1);
    auto i3 = mesh.getIndex(i + 2);

    const auto& v1 = mesh.getVertex(i1);
    const auto& v2 = mesh.getVertex(i2);
    const auto& v3 = mesh.getVertex(i3);
    // assuming CCW winding and righthanded
    auto n = glm::normalize(glm::cross(v2 - v1, v3 - v1));

    // switch normal direction if CW winding
    if (CW_winding) {
      n = -n;
    }

    normals[i1] += n;
    normals[i2] += n;
    normals[i3] += n;
  }

  // normalize normals
  for (size_t i = 0; i < mesh.getNumVertices(); ++i) {
    normals[i] = glm::normalize(normals[i]);
  }

  // replace original normals
  mesh.getNormals() = std::move(normals);
}

void swapWindingOrder(ofMesh& mesh)
{
  if (mesh.getMode() != OF_PRIMITIVE_TRIANGLES) {
    throw std::invalid_argument("The mode of input mesh must be OF_PRIMITIVE_TRIANGLES.");
  }

  auto indices = mesh.getIndices();
  for (std::size_t i = 0; i < mesh.getNumIndices(); i += 3) {
    std::swap(indices[i], indices[i + 1]);
  }
  mesh.getIndices() = std::move(indices);
}

void scaleMesh(ofMesh& mesh, float scale)
{
  for (std::size_t i = 0; i < mesh.getNumVertices(); ++i) {
    mesh.setVertex(i, mesh.getVertex(i) * scale);
  }
}

// Interal helper function for subdivideMesh.
static void subdivide(ofMesh& mesh, bool recalcNormal, bool CW_winding, bool normalizeVert)
{
  using Idx = decltype(mesh.getIndex(0));
  using Edge = std::pair<Idx, Idx>;

  // Hashing the edge pair using a symmetric hash
  static auto pairHash = [](const Edge& p) {
    auto hashIdx = std::hash<Idx> {};
    // note that this is symmetric
    return hashIdx(p.first) ^ hashIdx(p.second);
  };

  // (a,b) = (b,a)
  static auto pairCompare = [](const std::pair<Idx, Idx>& p1,
                               const std::pair<Idx, Idx>& p2) {
    return (p1.first == p2.first && p1.second == p2.second) ||
           (p1.first == p2.second && p1.second == p2.first);
  };

  // A pair of vertex idx -> midpoint idx
  // this will prevent duplicated vertices
  std::unordered_map<Edge, Idx, decltype(pairHash), decltype(pairCompare)>
      midpoints(0, pairHash, pairCompare);

  auto n = mesh.getNumIndices();
  // current vertex index
  Idx currV = mesh.getNumVertices();
  // we need to split one triangle into 4
  std::vector<Idx> newIndices;

  // add the midpoints of two vertex to mesh
  auto addMidpoint = [&](Idx i1, Idx i2) {
    auto status = midpoints.emplace(std::make_pair(i1, i2), currV);

    if (status.second) {
      // insert success, make a new vertex
      const auto& v1 = mesh.getVertex(i1);
      const auto& v2 = mesh.getVertex(i2);
      if (normalizeVert) {
        mesh.addVertex(glm::normalize(v1 + v2));
      } else {
        mesh.addVertex((v1 + v2) * 0.5f);
      }
      return currV++;
    } else {
      //otherwise, the vertex has been inserted before, return old result
      return status.first->second;
    }
  };

  for (std::size_t i = 0; i < n; i += 3) {
    auto i1 = mesh.getIndex(i);
    auto i2 = mesh.getIndex(i + 1);
    auto i3 = mesh.getIndex(i + 2);

    auto i12 = addMidpoint(i1, i2);
    auto i23 = addMidpoint(i2, i3);
    auto i13 = addMidpoint(i1, i3);

    if (CW_winding) {
      // Assuming CW winding order
      // top
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i1);
      newIndices.emplace_back(i13);
      // left
      newIndices.emplace_back(i2);
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i23);
      // mid
      newIndices.emplace_back(i23);
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i13);
      // right
      newIndices.emplace_back(i23);
      newIndices.emplace_back(i13);
      newIndices.emplace_back(i3);
    } else {
      // Assuming CCW winding order
      // top
      newIndices.emplace_back(i1);
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i13);
      // left
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i2);
      newIndices.emplace_back(i23);
      // mid
      newIndices.emplace_back(i12);
      newIndices.emplace_back(i23);
      newIndices.emplace_back(i13);
      // right
      newIndices.emplace_back(i13);
      newIndices.emplace_back(i23);
      newIndices.emplace_back(i3);
    }
  }
  mesh.getIndices() = std::move(newIndices);

  if (recalcNormal) {
    recalcNormals(mesh);
  }
}

void subdivideMesh(ofMesh& mesh, std::size_t iter, bool recalcNormal, bool CW_winding, bool normalizeVert)
{
  if (mesh.getMode() != OF_PRIMITIVE_TRIANGLES) {
    throw std::invalid_argument("The mode of input mesh must be OF_PRIMITIVE_TRIANGLES.");
  }

  for (std::size_t i = 0; i < iter; ++i) {
    subdivide(mesh, recalcNormal, CW_winding, normalizeVert);
  }
}

ofMesh makePlane(float width, float height, std::size_t xRes, std::size_t yRes)
{
  ofMesh mesh;

  // we only support triangle meshes.
  mesh.setMode(OF_PRIMITIVE_TRIANGLES);

  float xOffset = -width / 2, yOffset = -height / 2;
  float xScale = width / xRes, yScale = height / yRes;

  auto n = glm::vec3(0, 0, 1);

  // construct vertices and normal
  for (std::size_t x = 0; x < xRes; ++x) {
    for (std::size_t y = 0; y < yRes; ++y) {
      mesh.addVertex(glm::vec3(x * xScale + xOffset, y * yScale + yOffset, 0));
      mesh.addNormal(n);
    }
  }

  // add indices
  for (std::size_t x = 0; x < xRes - 1; ++x) {
    for (std::size_t y = 0; y < yRes - 1; ++y) {
      // CCW winding
      // bottomleft -> right -> upperright
      mesh.addTriangle(x * xRes + y, (x + 1) * xRes + y, (x + 1) * xRes + (y + 1));
      // bottomleft -> upperright -> up
      mesh.addTriangle(x * xRes + y, (x + 1) * xRes + (y + 1), x * xRes + (y + 1));
    }
  }

  return mesh;
}

ofMesh makeCube(float length)
{
  ofMesh mesh;

  float scaling = length / 2;

  mesh.addVertex(scaling * glm::vec3(-1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(1, 1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, 1, -1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, -1));

  mesh.addTriangle(0, 1, 2);
  mesh.addTriangle(2, 3, 0);
  mesh.addTriangle(1, 5, 6);
  mesh.addTriangle(6, 2, 1);
  mesh.addTriangle(7, 6, 5);
  mesh.addTriangle(5, 4, 7);
  mesh.addTriangle(4, 0, 3);
  mesh.addTriangle(3, 7, 4);
  mesh.addTriangle(4, 5, 1);
  mesh.addTriangle(1, 0, 4);
  mesh.addTriangle(3, 2, 6);
  mesh.addTriangle(6, 7, 3);

  recalcNormals(mesh);
  return mesh;
}

ofMesh makeKST(float length)
{
  ofMesh mesh;

  float scaling = length / 2;

  mesh.addVertex(scaling * glm::vec3(-1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, 1, -1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, -1));

  // slice
  mesh.addTriangle(1, 5, 2);

  mesh.addTriangle(0, 1, 2);
  mesh.addTriangle(1, 4, 5);
  mesh.addTriangle(2, 5, 6);

  mesh.addTriangle(0, 6, 3);
  mesh.addTriangle(0, 2, 6);

  mesh.addTriangle(0, 3, 4);
  mesh.addTriangle(0, 4, 1);

  mesh.addTriangle(4, 3, 6);
  mesh.addTriangle(4, 6, 5);

  recalcNormals(mesh);
  return mesh;
}

ofMesh makeKST2(float length)
{
  ofMesh mesh;

  float scaling = length / 2;

  mesh.addVertex(scaling * glm::vec3(-1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(1, -1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, 1));
  mesh.addVertex(scaling * glm::vec3(-1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, -1, -1));
  mesh.addVertex(scaling * glm::vec3(1, 1, -1));
  mesh.addVertex(scaling * glm::vec3(-1, 1, -1));

  mesh.addVertex(scaling * glm::vec3(1, 0, 1));
  mesh.addVertex(scaling * glm::vec3(0, 1, 1));
  mesh.addVertex(scaling * glm::vec3(1, 1, 0));

  mesh.addTriangle(7, 9, 8);

  mesh.addTriangle(0, 1, 7);
  mesh.addTriangle(0, 7, 8);
  mesh.addTriangle(0, 8, 2);

  mesh.addTriangle(4, 7, 1);
  mesh.addTriangle(4, 9, 7);
  mesh.addTriangle(4, 5, 9);

  mesh.addTriangle(6, 9, 5);
  mesh.addTriangle(6, 8, 9);
  mesh.addTriangle(6, 2, 8);

  mesh.addTriangle(0, 6, 3);
  mesh.addTriangle(0, 2, 6);

  mesh.addTriangle(0, 3, 4);
  mesh.addTriangle(0, 4, 1);

  mesh.addTriangle(4, 3, 6);
  mesh.addTriangle(4, 6, 5);

  recalcNormals(mesh);
  return mesh;
}

ofMesh makeKST3(float length)
{
  ofMesh mesh = makeCube(length);
  subdivideMesh(mesh);

  using Idx = decltype(mesh.getIndex(0));
  std::vector<Idx> newIndices;

  // the corner to be discarded
  static const Idx IDX_CUT = 2;
  // neighbors
  static const Idx IDX_LEFT = 8;
  static const Idx IDX_RIGHT = 10;
  static const Idx IDX_BELOW = 15;
  static const Idx IDX_TOP = 9;

  // discard the index
  mesh.removeVertex(IDX_CUT);

  // recalc faces
  for (size_t i = 0; i < mesh.getNumIndices(); i += 3) {
    Idx i1 = mesh.getIndex(i);
    Idx i2 = mesh.getIndex(i + 1);
    Idx i3 = mesh.getIndex(i + 2);
    // discard face
    if (i1 == IDX_CUT || i2 == IDX_CUT || i3 == IDX_CUT) {
      continue;
    }
    // recalculate face index
    i1 -= i1 < IDX_CUT ? 0 : 1;
    i2 -= i2 < IDX_CUT ? 0 : 1;
    i3 -= i3 < IDX_CUT ? 0 : 1;
    newIndices.emplace_back(i1);
    newIndices.emplace_back(i2);
    newIndices.emplace_back(i3);
  }
  // missing face
  newIndices.emplace_back(IDX_TOP);
  newIndices.emplace_back(IDX_LEFT);
  newIndices.emplace_back(IDX_RIGHT);

  // extra face
  newIndices.emplace_back(IDX_LEFT);
  newIndices.emplace_back(IDX_BELOW);
  newIndices.emplace_back(IDX_RIGHT);

  mesh.getIndices() = std::move(newIndices);

  recalcNormals(mesh);
  return mesh;
}

/// ref: section 9.4 of Schneider and Eberly, Geometric Tools for Computer
/// Graphics, 2003
ofMesh makeTetrahedron(float radius)
{
  ofMesh mesh;

  static const float sqrt2 = sqrt(2);
  static const float sqrt6 = sqrt(6);

  mesh.addVertex(radius * glm::vec3(0, 0, 1));
  mesh.addVertex(radius * glm::vec3(2 * sqrt2 / 3, 0, -1 / 3));
  mesh.addVertex(radius * glm::vec3(-sqrt2 / 3, sqrt6 / 3, -1 / 3));
  mesh.addVertex(radius * glm::vec3(-sqrt2 / 3, -sqrt6 / 3, -1 / 3));

  mesh.addTriangle(0, 1, 2);
  mesh.addTriangle(0, 2, 3);
  mesh.addTriangle(0, 3, 1);
  mesh.addTriangle(1, 3, 2);

  recalcNormals(mesh);
  return mesh;
}

/// ref: section 9.4 of Schneider and Eberly, Geometric Tools for Computer
/// Graphics, 2003
ofMesh makeOctahedron(float radius)
{
  ofMesh mesh;

  mesh.addVertex(radius * glm::vec3(1, 0, 0));
  mesh.addVertex(radius * glm::vec3(-1, 0, 0));
  mesh.addVertex(radius * glm::vec3(0, 1, 0));
  mesh.addVertex(radius * glm::vec3(0, -1, 0));
  mesh.addVertex(radius * glm::vec3(0, 0, 1));
  mesh.addVertex(radius * glm::vec3(0, 0, -1));

  mesh.addTriangle(4, 0, 2);
  mesh.addTriangle(4, 2, 1);
  mesh.addTriangle(4, 1, 3);
  mesh.addTriangle(4, 3, 0);
  mesh.addTriangle(5, 2, 0);
  mesh.addTriangle(5, 1, 2);
  mesh.addTriangle(5, 3, 1);
  mesh.addTriangle(5, 0, 3);

  recalcNormals(mesh);
  return mesh;
}

/// ref: section 9.4 of Schneider and Eberly, Geometric Tools for Computer
/// Graphics, 2003
ofMesh makeIcosahedron(float radius)
{
  ofMesh mesh;
  // golden ratio
  static const float t = (1 + sqrt(5)) / 2;
  // inverse norm * radius = r/sqrt(1+t^2)
  float scaling = radius / std::sqrt(t * t + 1);

  mesh.addVertex(scaling * glm::vec3(t, 1, 0));
  mesh.addVertex(scaling * glm::vec3(-t, 1, 0));
  mesh.addVertex(scaling * glm::vec3(t, -1, 0));
  mesh.addVertex(scaling * glm::vec3(-t, -1, 0));
  mesh.addVertex(scaling * glm::vec3(1, 0, t));
  mesh.addVertex(scaling * glm::vec3(1, 0, -t));
  mesh.addVertex(scaling * glm::vec3(-1, 0, t));
  mesh.addVertex(scaling * glm::vec3(-1, 0, -t));
  mesh.addVertex(scaling * glm::vec3(0, t, 1));
  mesh.addVertex(scaling * glm::vec3(0, -t, 1));
  mesh.addVertex(scaling * glm::vec3(0, t, -1));
  mesh.addVertex(scaling * glm::vec3(0, -t, -1));

  mesh.addTriangle(0, 8, 4);
  mesh.addTriangle(0, 5, 10);
  mesh.addTriangle(2, 4, 9);
  mesh.addTriangle(2, 11, 5);
  mesh.addTriangle(1, 6, 8);
  mesh.addTriangle(1, 10, 7);
  mesh.addTriangle(3, 9, 6);
  mesh.addTriangle(3, 7, 11);
  mesh.addTriangle(0, 10, 8);
  mesh.addTriangle(1, 8, 10);
  mesh.addTriangle(2, 9, 11);
  mesh.addTriangle(3, 11, 9);
  mesh.addTriangle(4, 2, 0);
  mesh.addTriangle(5, 0, 2);
  mesh.addTriangle(6, 1, 3);
  mesh.addTriangle(7, 3, 1);
  mesh.addTriangle(8, 6, 4);
  mesh.addTriangle(9, 4, 6);
  mesh.addTriangle(10, 5, 7);
  mesh.addTriangle(11, 7, 5);

  recalcNormals(mesh);
  return mesh;
}

ofMesh makeIcosphere(float radius, std::size_t iterations)
{
  // first generate a icosahedron of radius 1
  ofMesh mesh = makeIcosahedron(1);

  // Then subdivide
  subdivideMesh(mesh, iterations, false, false, true);

  // and scale the mesh
  scaleMesh(mesh, radius);

  // finally, calculate normals
  // an alternative would to be to use the normalized (x,y,z) coordinates of
  // the mesh, but we will be deforming it anyways.
  recalcNormals(mesh);

  return mesh;
}
