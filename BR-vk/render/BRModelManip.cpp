#include <BRModelManip.h>

using namespace BR;

ModelManip::ModelManip()
{
    reset();
}

void ModelManip::reset()
{
    m_scale = glm::vec3( 1 );
    m_translate = glm::vec3( 0 );
    m_rotateX = 0;
    m_rotateY = 0;
    m_rotateZ = 0;

    m_mat = glm::mat4( 1 );
}

//Scale, rotate, translate, in that order
void ModelManip::doManip( int x, int y, ManipMode mode )
{
    //true - positive, false - negative
    bool direction = x - m_mousePos.x > 0 ? true : false;
    bool recalculate = false;

    if ( mode == translate )
    {
        doTranslate( static_cast<Axis>( m_mouseButton ), direction );
        recalculate = true;
        setMousePos( x, y );
    }
    else if ( mode == rotate )
    {
        doRotate( static_cast<Axis>( m_mouseButton ), direction );
        recalculate = true;
        setMousePos( x, y );
    }
    else if ( mode == scale )
    {
        doScale( static_cast<Axis>( m_mouseButton ), direction );
        recalculate = true;
        setMousePos( x, y );
    }

    if ( recalculate )
    {
        m_mat = glm::mat4( 1 );
        m_mat =
            glm::rotate( m_mat, glm::radians( static_cast<float>( m_rotateX ) ),
                         glm::vec3( 1.0f, 0.0f, 0.0f ) );
        m_mat =
            glm::rotate( m_mat, glm::radians( static_cast<float>( m_rotateY ) ),
                         glm::vec3( 0.0f, 1.0f, 0.0f ) );
        m_mat =
            glm::rotate( m_mat, glm::radians( static_cast<float>( m_rotateZ ) ),
                         glm::vec3( 0.0f, 0.0f, 1.0f ) );
        m_mat = glm::scale( m_mat, m_scale );
        m_mat = glm::translate( m_mat, m_translate );
    }
}

void ModelManip::doTranslate( Axis axis, bool direction )
{
    int mult = direction ? 1 : -1;

    switch ( axis )
    {
        case X:
            m_translate.x += mult * 0.01f;
            break;
        case Y:
            m_translate.y += mult * 0.01f;
            break;
        case Z:
            m_translate.z += mult * 0.01f;
            break;
        default:
            break;
    }
}

void ModelManip::doRotate( Axis axis, bool direction )
{
    int mult = direction ? 1 : -1;

    switch ( axis )
    {
        case X:
            m_rotateX += mult * 5;
            break;
        case Y:
            m_rotateY += mult * 5;
            break;
        case Z:
            m_rotateZ += mult * 5;
            break;
        default:
            break;
    }
}

void ModelManip::doScale( Axis axis, bool direction )
{
    int mult = direction ? 1 : -1;

    switch ( axis )
    {
        case X:
            m_scale.x += mult * 0.01f;
            break;
        case Y:
            m_scale.y += mult * 0.01f;
            break;
        case Z:
            m_scale.z += mult * 0.01f;
            break;
        default:
            break;
    }
}