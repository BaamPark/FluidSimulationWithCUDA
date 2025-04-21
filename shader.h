#pragma once
#include <string>
#include <GL/glew.h>

class Shader {
public:
    GLuint ID;

    Shader(); // No parameters now
    void use();

    void setMat4(const std::string &name, const float* mat) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
};
