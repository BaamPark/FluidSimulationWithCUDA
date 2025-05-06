#include "shader.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iterator>

#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>            // or your GL loader
#include "shader.h"

// ─── File loader ──────────────────────────────────────────────
static std::string loadFileToString(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "ERROR: could not open file '" << path << "'\n";
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// ─── Shader compile/link error checker ───────────────────────
static void checkCompileErrors(GLuint handle, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    if (type == "PROGRAM") {
        glGetProgramiv(handle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(handle, 1024, nullptr, infoLog);
            std::cerr << "PROGRAM_LINK_ERROR:\n" << infoLog << "\n";
        }
    } else {
        glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(handle, 1024, nullptr, infoLog);
            std::cerr << type << "_COMPILE_ERROR:\n" << infoLog << "\n";
        }
    }
}
// ───────────────────────────────────────────────────────────────




Shader::Shader() {
    // Vertex shader source: outputs world position and foam factor, sets point size for sphere radius
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aFoam;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform float screenHeight;
        uniform float fov;
        out vec3 WorldPos;
        out float Foam;
        void main() {
            // Compute world position of the particle (after model transform)
            vec4 worldPos4 = model * vec4(aPos, 1.0);
            WorldPos = worldPos4.xyz;
            // Transform to view space for projection
            vec4 viewPos4 = view * worldPos4;
            // Set gl_Position for this point
            gl_Position = projection * viewPos4;
            // Compute point sprite size so that each particle covers a sphere of constant world radius
            float dist = -viewPos4.z; // distance from camera (view space z is negative in front of camera)
            float radius = 0.01;      // radius of each particle sphere in world units (tunable)
            // Calculate point size in pixels based on perspective (projected sphere diameter)
            float pixelScale = screenHeight / tan(radians(fov * 0.5));
            gl_PointSize = (radius * pixelScale) / dist;
            // Pass foam factor to fragment shader
            Foam = aFoam;
        }
    )";

    // Fragment shader source: computes lighting, reflection, refraction, and foam for each particle fragment
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 WorldPos;
        in float Foam;
        uniform mat4 view;      // view matrix (for transforming normals if needed)
        uniform mat4 invView;   // inverse of view matrix (to transform normals to world)
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 waterColor = vec3(0.1, 0.4, 0.6);
        uniform samplerCube skybox;
        void main() {
            // Reconstruct normalized device coordinates of this fragment within the point sprite
            // gl_PointCoord ranges from 0 to 1 across the sprite
            vec2 uv = gl_PointCoord * 2.0 - 1.0;      // map to [-1, 1]
            float r2 = uv.x*uv.x + uv.y*uv.y;
            if (r2 > 1.0) {
                // Discard fragments outside the circle (to make a round particle)
                discard;
            }
            // Compute the normal vector of the sphere at this fragment (in view space)
            float zComponent = sqrt(1.0 - r2);
            vec3 normalView = vec3(uv.x, uv.y, zComponent);
            // Transform normal to world space (ignore translation, use invView rotation)
            vec3 normalWorld = normalize((invView * vec4(normalView, 0.0)).xyz);
            // Compute fragment position in world space (approximate as particle center + normal * radius)
            float radius = 0.01; // must match the radius used in vertex shader
            vec3 fragPosWorld = WorldPos + normalWorld * radius;
            // Compute view direction and light direction (in world space)
            vec3 viewDir = normalize(viewPos - fragPosWorld);   // from fragment toward camera
            vec3 lightDir = normalize(lightPos - fragPosWorld); // from fragment toward light source

            // --- Lighting calculations ---
            // Ambient (small base light)
            vec3 ambient = 0.5 * lightColor * waterColor;
            // Diffuse (Lambertian)
            float diff = max(dot(normalWorld, lightDir), 0.0);
            vec3 diffuse = 0.6 * diff * lightColor * waterColor;
            // Specular (Blinn-Phong)
            vec3 reflectDir = reflect(-lightDir, normalWorld);
            float specStrength = 0.2;
            float shininess = 32.0;
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specStrength * spec * lightColor;

            // --- Environment reflection & refraction ---
            // Compute reflection and refraction vectors for environment mapping
            vec3 I = normalize(fragPosWorld - viewPos);  // incident view ray (from camera to frag)
            vec3 reflectVec = reflect(I, normalWorld);
            // Refractive index of water ~1.33, compute refraction (air->water)
            vec3 refractVec = refract(I, normalWorld, 1.0/1.33);
            // Sample the environment cubemap for reflection and refraction colors
            vec3 reflectColor = texture(skybox, reflectVec).rgb;
            vec3 refractColor = texture(skybox, refractVec).rgb;
            // vec3 reflectColor = vec3(0.8, 0.9, 1.0);
            // vec3 refractColor = waterColor * 0.6;

            // Apply water tint to refracted color (simulate absorption)
            refractColor *= waterColor;
            // Fresnel factor for reflectance (using Schlick's approximation)
            float cosTheta = max(dot(normalWorld, -I), 0.0);
            float fresnelFactor = 0.04 + (1.0 - 0.04) * pow(1.0 - cosTheta, 5.0);
            // Mix reflection and refraction based on Fresnel (angle-dependent)
            vec3 envColor = mix(refractColor, reflectColor, pow(fresnelFactor, 0.3));

            // --- Combine lighting and environment ---
            vec3 baseColor = ambient + diffuse + specular;
            vec3 finalColor = baseColor + envColor;
            finalColor *= mix(1.0, 0.6, 1.0 - cosTheta);

            // --- Foam effect ---
            // Mix in foam (white) based on foam factor
            finalColor = mix(finalColor, vec3(1.0, 1.0, 1.0), pow(Foam, 1.5));
            // Increase opacity if foam is present (foam makes water more opaque)
            float baseAlpha = 0.2;
            float alpha = mix(baseAlpha, 1.0, Foam);

            FragColor = vec4(finalColor, alpha);
        }
    )";




    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Check compilation success
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
    }

    // Link shaders into a program
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
    }

    // Shaders no longer needed after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// New constructor, ~line 30
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    // 1. retrieve the source from file
    std::string vertSource = loadFileToString(vertexPath);
    std::string fragSource = loadFileToString(fragmentPath);
    const char* vS = vertSource.c_str();
    const char* fS = fragSource.c_str();

    // 2. compile & link exactly as your existing code does
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vS, nullptr);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fS, nullptr);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}




void Shader::use() {
    glUseProgram(ID);
}

void Shader::setMat4(const std::string &name, const float* mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}