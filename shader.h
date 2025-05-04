#pragma once
#include <string>
#include <GL/glew.h>

class Shader {
public:
    GLuint ID;
    Shader();  // Constructor compiles and links the water shaders
    void use();
    // Utility uniform functions
    void setMat4(const std::string &name, const float* mat) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setFloat(const std::string &name, float value) const;
    void setInt(const std::string &name, int value) const;
};
