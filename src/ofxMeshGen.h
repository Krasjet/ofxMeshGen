#pragma once

#include <ofMain.h>

/// \brief Re-calculate the normals for a mesh.
///
/// This function will recalculate the normal vectors of an `ofMesh` object.
/// It is based on a simple cross product within each faces of the triangle so
/// it won't accurately calculate the true normals of a geometry, but it's
/// usually good enough.
///
/// By default, the function would assume `mesh` has counterclock-wise (CCW)
/// winding order. If your mesh has clockwise (CW) winding order, set
/// `CW_winding` to true.
///
/// The default generators of openFrameworks will generate meshes with
/// inconsistent winding orders [1], but most of them are in CW order. The
/// generators provided by this addon, however, will have consistent CCW
/// winding order. Also check out the `swapWindingOrder` function.
///
/// The mode of `mesh` must be `OF_PRIMITIVE_TRIANGLES`.
///
/// [1]: https://github.com/openframeworks/openFrameworks/issues/2676
///
/// \param mesh The mesh to be processed.
/// \param CW_winding Use clockwise winding order instead.
void recalcNormals(ofMesh& mesh, bool CW_winding = false);

/// \brief Swap the winding order of a mesh.
///
/// The mode of `mesh` must be `OF_PRIMITIVE_TRIANGLES`.
///
/// \param mesh The mesh to be processed.
void swapWindingOrder(ofMesh& mesh);

/// \brief Scale the vertex position by `scale`.
///
/// \param mesh The mesh to be processed.
/// \param scale The scaling factor
void scaleMesh(ofMesh& mesh, float scale);

/// \brief Subdivide each triangle in the mesh using midpoints
///
/// The subdivision will not create any duplicated vertices.
///
/// By default, the function would assume `mesh` has counterclock-wise (CCW)
/// winding order. If your mesh has clockwise (CW) winding order, set
/// `CW_winding` to true.
///
/// \param mesh The mesh to be processed.
/// \param iter The number of iterations. In each iteration a single triangle will be divided into 4.
/// \param recalcNormal if set to true, normal vectors will be recalculated.
/// \param CW_winding If set to true, use clockwise winding instead.
/// \param normalizeVert It set to true, the coordinates of each the mid point
///        will be normalized to 1. This is useful for generating subdivided spheres.
void subdivideMesh(ofMesh& mesh, std::size_t iter = 1, bool recalcNormal = true, bool CW_winding = false, bool normalizeVert = false);

/// \brief Generate a plane with given width and height.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param width The width of the plane.
/// \param height The height of the plane.
/// \param xRes The number of vertices in x direction.
/// \param yRes The number of vertices in y direction.
ofMesh makePlane(float width = 100.0f, float height = 100.0f, std::size_t xRes = 300, std::size_t yRes = 300);

/// \brief Generate a cube with side length.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param length The side length of the cube.
ofMesh makeCube(float length = 100);

/// \brief Generate a tetrahedron with circumscribed radius.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param radius The radius of the circumscribed sphere.
ofMesh makeTetrahedron(float radius = 40);

/// \brief Generate an octahedron with circumscribed radius.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param radius The radius of the circumscribed sphere.
ofMesh makeOctahedron(float radius = 40);

/// \brief Generate an icosahedron with circumscribed radius.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param radius The radius of the circumscribed sphere.
ofMesh makeIcosahedron(float radius = 40);

/// \brief Generate an icosphere with radius.
///
/// Unlike the `ofMesh::icosphere`, the icosphere generated with function will
/// not have duplicated vertices, this is useful if you want to perform vertex
/// based deformation later.
///
/// The winding order of the generated mesh will be counterclock-wise (CCW).
///
/// \param radius The radius of the sphere.
/// \param iterations This will determine the smoothness of the sphere.
ofMesh makeIcosphere(float radius = 40, std::size_t iterations = 5);
