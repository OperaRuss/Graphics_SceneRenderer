//
// Created by russ on 11/17/21.
//

#ifndef ASSIGNMENT_4_LIGHT_H
#define ASSIGNMENT_4_LIGHT_H

#endif //ASSIGNMENT_4_LIGHT_H

struct Light{
    glm::vec3 _position;
    glm::vec3 _ambient;
    glm::vec3 _diffuse;
    glm::vec3 _specular;
    unsigned int colorWheel = 0;

    Light() {
        _position = glm::vec3(0.0f, 3.0f, 0.0f);
        _ambient = glm::vec3(1.0f,1.0f,1.0f);
        _diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        _specular = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    Light(glm::vec3 pos, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec) :
        _position(pos), _ambient(amb), _diffuse(diff), _specular(spec) {}

    void changeColor(const std::vector<glm::vec3> &rainbow){
        glm::vec3 gray = glm::vec3(glm::dot(rainbow[colorWheel], glm::vec3(0.2126, 0.7152, 0.0722)));
        glm::vec3 lightColor = glm::mix(rainbow[colorWheel], gray, 0.75);

        _ambient = 0.1f * lightColor;
        _diffuse = 0.8f * lightColor;
        _specular = lightColor;

        ++colorWheel %= rainbow.size();
    }

    void reset(){
        _position = glm::vec3(0.0f, 10.0f, 0.0f);
        _ambient = glm::vec3 (0.15f);
        _diffuse = glm::vec3 (0.5f);
        _specular = glm::vec3 (0.7f);
    }
};