#pragma once

#include <BRManip.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace BR
{
class CameraManip : public Manip
{
   public:
    CameraManip();
    ~CameraManip(){};

    void doManip( int x, int y );
    void reset() override;

   private:
    glm::vec3 m_eye;
    glm::vec3 m_at;
    glm::vec3 m_up;

    float m_distance;
    float m_angleX;
    float m_angleY;

    void orbit( int x, int y );
    void zoom( int x, int y );
    void pan( int x, int y );
};

}  // namespace BR
