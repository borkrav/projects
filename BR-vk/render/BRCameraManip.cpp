#include <BRCameraManip.h>

using namespace BR;

CameraManip::CameraManip()
{
    reset();
}

void CameraManip::reset()
{
    m_eye = glm::vec3( 0, 0, 8 );
    m_at = glm::vec3( 0, 0, 0 );
    m_up = glm::vec3( 0, 1, 0 );
    m_distance = 8;
    m_angleX = 0;
    m_angleY = 0;
    m_mat = lookAt( m_eye, m_at, m_up );
}

void CameraManip::doManip( int x, int y )
{
    if ( m_mouseButton == lmb )
        orbit( x, y );
    else if ( m_mouseButton == mmb )
        pan( x, y );
    else if ( m_mouseButton == rmb )
        zoom( x, y );
}

glm::vec3& CameraManip::getEye()
{
    return m_eye;
}

void CameraManip::orbit( int x, int y )
{
    float distanceX = x - m_mousePos.x;
    float distanceY = y - m_mousePos.y;

    //up/down moves around the circle, on the y/z plane
    //the longitude of a sphere
    m_angleY += distanceY * 0.3;
    float deltaY = sin( glm::radians( m_angleY ) ) * m_distance;
    float deltaZ = cos( glm::radians( m_angleY ) ) * m_distance;

    if ( ( static_cast<int>( m_angleY ) % 360 > 90 ) &&
         ( static_cast<int>( m_angleY ) % 360 < 270 ) )
        m_up.y = -1;
    else
        m_up.y = 1;

    m_eye = glm::vec3( m_eye.x, deltaY, deltaZ );

    //left/right moves around the circle, on the x/z plane, relative to the y position
    //the latitude of a sphere

    m_angleX += distanceX * 0.3;
    float deltaX = sin( glm::radians( m_angleX ) ) * abs( deltaZ );

    deltaZ = cos( glm::radians( m_angleX ) ) * abs( deltaZ );

    if ( ( static_cast<int>( m_angleX ) % 360 > 90 ) &&
         ( static_cast<int>( m_angleX ) % 360 < 270 ) )
    {
        deltaZ = -deltaZ;
        deltaX = -deltaX;
    }

    m_eye = glm::vec3( deltaX, m_eye.y, deltaZ );

    m_mat = lookAt( m_eye, m_at, m_up );

    setMousePos( x, y );
}

void CameraManip::zoom( int x, int y )
{
    float distanceX = x - m_mousePos.x;
    float distanceY = y - m_mousePos.y;
    int sign = 1;

    float zoomDistance = sqrt( pow( distanceX, 2 ) + pow( distanceY, 2 ) );

    if ( distanceX > 0 )
        sign = -1;
    else
        sign = 1;

    m_eye *= 1 + sign * zoomDistance * 0.05;

    m_distance = glm::length( m_at - m_eye );

    m_mat = lookAt( m_eye, m_at, m_up );

    setMousePos( x, y );
}

void CameraManip::pan( int x, int y )
{
    float distanceX = x - m_mousePos.x;
    float distanceY = y - m_mousePos.y;

    m_eye += glm::vec3( distanceX * 0.01, distanceY * 0.01, 0 );
    m_at += glm::vec3( distanceX * 0.01, distanceY * 0.01, 0 );

    m_mat = lookAt( m_eye, m_at, m_up );

    setMousePos( x, y );
}

/*
    LookAt


   //Affine transformation preserves colinearity (all points lying on a line still like on a line) and ratios
    //of distances (midpoint of a line segment remains the midpoint of a line segment)

    //We create an affine transformation matrix from camera to world space. Inverse of this is the transformation
    //from the world to the camera space

    //World space
    //x - east
    //y - up
    //z - south

    //Camera space
    //x - right
    //y - up
    //z - away from camera (from eye into scene, negative Z relative to world space)

    // construct orthonormal basis vectors to describe the camera space
    // orthonormal - perpendicular to one another
    // basis - any vector in the space can be written as a linear combination of the basis vectors

    //vector from point to camera, this is Z axis
    glm::vec3 zAxis = glm::normalize( pos - cam );

    //xAxis, perpendicular to both zAxis and up vector
    glm::vec3 xAxis = glm::normalize( glm::cross( zAxis, up ) );

    //yAxis ends up being xAxis cross zAxis.
    glm::vec3 yAxis = glm::cross( xAxis, zAxis );

    //Need to negate the zAxis to ensure it points away from the camera, by convension
    zAxis = -zAxis;


    //camera to world space - affine transformation - linear transformation (R) followed by a translation (T)
    // M = T*R
    
    // We inverse becase we're computing model->camera, not camera->model, however our inputs are camera basis vectors
    // inverse M = (TR)^-1 = R^-1 T^-1

    //R is the orthonormal basis vectors

    // R is transpose, because R is orthogonal
    // T - get the inverse
    // multiply, and we get the matrix


    return glm::lookAt( cam, pos, up );
*/

//At a high level, I think this is how it works

/* 
    * Vertices/points are in 3d space, with basis vectors i,j,k, (0,0,1) (0,1,0) (1,0,0) ie identity
    * Model matrix transforms the 3d space, as a result we have our vertices/points in the new positions - a series of translations, rotations and scales
    * View matrix transforms the 3d space to the POV of the camera, and translates the 3d space to the camera's position - linear transformation followed by a translation
    * Projection matrix transforms what's in the camera space to "screen space", flattening it into 2d
*/

//Math lesson

// https://www.youtube.com/watch?v=XkY2DOUCWMU

//basis vectors define the coordinate system
// [2,3] is defined as [2*i, 3*j] where i is i unit in the X axis, j is one unit in the Y axis
// this is a linear combination, you're combining two vectors

// the set of all linear combinations reacheable by a set of vectors is the span

//Every set of basis vector yields every possible vector

// Linearly dependant vectors means nothing is added to the span
// In other words, both vectors are on the same line, or all 3 vectors are on the same plane

//Linear transformations - lines stay lines, origin stay fixed

//A linear transformation can be described by how the basis vectors are transformed

//This makes sense, a linear transformation from the world space to camera space would only be dependant on the new basis vectors

// A matrix is a transformation of space, this is all it is
// Every column is a basis vector
// a b c
// d e f
// g h i

// x -> a,d,g
// y -> b,e,h
// z -> c,f,i

// Multiplying two matrices -> apply one transformation, then apply another transformation
// read right to left

// The determinant is how much a unit square (1x1, defined by basis vectors) is scaled by the transformation
// How much is the space stretched/shrunk?
// Det is 0 if the transformation squishes the space onto a line - you lose a dimension
// Negative determinant flips the space (invert the orientation of space)
// j to the left of i -> j to the right of i
// Negative determinant in 3d implies that we went from right hand to left hand coordinate system

// Transform into a line - rank 1
// Transform into a plane - rank 2
// How many dimensions you have after transformation