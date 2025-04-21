#include "shader.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

// Shader::Shader() {
//     const char* vertexShaderSource = R"(
//         #version 330 core
//         layout(location = 0) in vec3 aPos;

//         uniform mat4 projection;
//         uniform mat4 view;

//         void main() {
//             gl_Position = projection * view * vec4(aPos, 1.0);
//             gl_PointSize = 4.0;
//         }
//     )";

Shader::Shader() {
    const char* vertexShaderSource = R"(
        #version 330 core
       
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;

        void main(){
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;

        gl_Position = projection * view * vec4(FragPos, 1.0);
        gl_PointSize = 6.0;
        }
    )";
    


    // const char* fragmentShaderSource = R"(
    //     #version 330 core
    //     out vec4 FragColor;

    //     void main() {
    //         FragColor = vec4(0.2, 0.6, 1.0, 1.0); // Light blue
    //     }
    // )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec4 FragColor;

        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;

        void main(){
        
            // Ambient
            float ambientStrength * lightColor;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Specular
            float specularStrength = 0.5;
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, 1.0);

        }

    )";

    int success = 0;
    char infoLog[1024];
    

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);  // Check whether complie successfully
    if(!success){
        glGetShaderInfoLog(vertex, 1024, nullptr, infoLog);
        std::cout<<"Error: SHADER COMPILE ERROR -- vertex" << "\n" << infoLog << std::endl;
    }
   

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);  // Check whether complie successfully
    if(!success){
        glGetShaderInfoLog(fragment, 1024, nullptr, infoLog);
        std::cout<<"Error: SHADER COMPILE ERROR -- fragment" << "\n" << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setMat4(const std::string& name, const float* mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);

}