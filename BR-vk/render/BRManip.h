#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace BR
{
class Manip
{
   public:
    Manip();
    ~Manip(){};

    enum MouseButton
    {
        lmb,
        mmb,
        rmb
    };

    void setMousePos( int x, int y );
    void setMouseButton( MouseButton button );

    glm::mat4& getMat();

    virtual void reset() = 0;

   protected:
    struct Position
    {
        int x;
        int y;
    };

    glm::mat4 m_mat;
    Position m_mousePos;
    MouseButton m_mouseButton;
};

}  // namespace BR
