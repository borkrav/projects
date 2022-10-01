#include <BRManip.h>

using namespace BR;

Manip::Manip() : m_mat( 1 )
{
}

glm::mat4& Manip::getMat()
{
    return m_mat;
}

void Manip::setMouseButton( MouseButton button )
{
    m_mouseButton = button;
}

void Manip::setMousePos( int x, int y )
{
    m_mousePos.x = x;
    m_mousePos.y = y;
}
