#pragma once

#include <BRManip.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace BR
{
class ModelManip : public Manip
{
   public:
    ModelManip();
    ~ModelManip(){};

    enum ManipMode
    {
        rotate,
        translate,
        scale
    };

    void doManip( int x, int y, ManipMode mode );

    void reset() override;

   private:
    enum Axis
    {
        X,
        Y,
        Z
    };

    glm::vec3 m_translate;
    glm::vec3 m_translateOld;
    glm::vec3 m_scale;
    int m_rotateX;
    int m_rotateY;
    int m_rotateZ;

    void doTranslate( Axis axis, bool direction );
    void doRotate( Axis axis, bool direction );
    void doScale( Axis axis, bool direction );
};

}  // namespace BR
