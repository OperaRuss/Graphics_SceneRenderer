//
// Created by russ on 11/17/21.
//

#ifndef ASSIGNMENT_4_MATERIAL_H
#define ASSIGNMENT_4_MATERIAL_H

struct Material{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    GLfloat   shininess;
    bool      reflective;
    unsigned int colorWheel = 0;

    Material(){
        ambient = glm::vec3(0.5f);
        diffuse = glm::vec3(0.5f);
        specular = glm::vec3(0.1f);
        shininess = 30.0f;
        reflective = false;
    }
    Material(bool reflective){
        ambient = glm::vec3(1.0f);
        diffuse = glm::vec3(1.0f);
        specular = glm::vec3(1.0f);
        shininess = 1200.0f;
        reflective = true;
    }
    Material(glm::vec3 color, GLfloat shine){
        ambient = 0.9f * color;
        diffuse = color;
        specular = glm::clamp(1.5f * color, 0.0f, 1.0f);
        shininess = shine;
        reflective = false;
    }
    Material(const glm::vec3& ambient, const glm::vec3& diffuse,
             const glm::vec3& specular, GLfloat shininess, bool reflective) :
            ambient(ambient), diffuse(diffuse), specular(specular),
            shininess(shininess), reflective(reflective) {}

    Material& operator=(const Material& other){
        if (this == &other)
            return *this;
        this->ambient = other.ambient;
        this->diffuse = other.diffuse;
        this->specular = other.specular;
        this->shininess = other.shininess;
        this->reflective = other.reflective;

        return *this;
    }

    friend bool operator==(const Material& ls, const Material& rs){
        if(ls.shininess != rs.shininess)
            return false;
        if(ls.reflective != rs.reflective)
            return false;
        for(int i = 0; i < 3; ++i){
            if (ls.ambient[i] != rs.ambient[i])
                return false;
            if (ls.diffuse[i] != rs.diffuse[i])
                return false;
            if (ls.specular[i] != rs.specular[i])
                return false;
        }
        return true;
    }

    void reset(){
        if(reflective)
            reflective = !reflective;
        ambient = 0.9f * glm::vec3(0.5);
        diffuse = glm::vec3(0.5);
        specular = glm::clamp(1.5f * glm::vec3(0.5), 0.0f, 1.0f);
    }

    void updateColor(const glm::vec3& color){
        ambient = 0.9f * color;
        diffuse = color;
        specular = glm::clamp(1.5f * color, 0.0f, 1.0f);
    }

    void updateColor(const std::vector<glm::vec3>& rainbow){
        ambient = 0.9f * rainbow[colorWheel];
        diffuse = rainbow[colorWheel];
        specular = glm::clamp(1.5f * rainbow[colorWheel], 0.0f, 1.0f);
        ++colorWheel %= rainbow.size();
    }

    void updateShininess(float shine){
        shininess = shine;
    }

    void toggleChrome(){
        reflective = !reflective;
    }

};



#endif //ASSIGNMENT_4_MATERIAL_H
