//
// Created by russ on 10/28/21.
//

#ifndef ASSIGNMENT_3_SHADER_H
#define ASSIGNMENT_3_SHADER_H

#include <string>
#include <iostream>
#include <GL/glew.h>
#include <glm/glm.hpp>

//todo add read in from text file
class Shader {
    // Much of this code was based on the Learn OpenGL tutorial at www.learnopengl.com
    // by Joey Devries.  While much of this is a direct transcription of his code,
    // I do so because it is both economical and guaranteed to work.  Several tweaks are
    // introduced to fit with my implementation.
public:
    GLuint programID;
    GLuint vShaderID;
    GLuint fShaderID;

    Shader(const char* vertexSource, const char* fragmentSource){
        // set up vertex shader
        vShaderID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vShaderID, 1, &vertexSource, NULL);
        glCompileShader(vShaderID);
        checkCompileErrors(vShaderID, "VERTEX");

        // set up fragment shader
        fShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fShaderID, 1, &fragmentSource, NULL);
        glCompileShader(fShaderID);
        checkCompileErrors(fShaderID, "FRAGMENT");

        // create the program
        programID = glCreateProgram();
        glAttachShader(programID, vShaderID);
        glAttachShader(programID, fShaderID);
        glLinkProgram(programID);
        checkCompileErrors(programID, "PROGRAM");
        glDeleteShader(vShaderID);
        glDeleteShader(fShaderID);
    }
    ~Shader(){
        glDeleteProgram(programID);
    }

    void use() {
        // Sets the shader to active
        glUseProgram(programID);
    }

    void terminate(){
        glDeleteProgram(programID);
    }

    // Uniform setting functions
    void setUniformBool(const std::string& name, const bool value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
    }
    void setUniformInt(const std::string& name, const int value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }
    void setUniformFloat(const std::string& name, const float value) const {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }
    void setUniformVec3(const std::string& name, const glm::vec3 value) const {
        glUniform3fv(glGetUniformLocation(programID, name.c_str()),1,&value[0]);
    }
    void setUniformMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
private:
    static void checkCompileErrors(GLuint shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if(type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR::" << type << "\n" << infoLog << std::endl;
            }
        }
        else{
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success){
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::PROGRAM_LINKING_ERROR::" << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

#endif //ASSIGNMENT_3_SHADER_H
