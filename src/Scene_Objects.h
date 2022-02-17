//
// Created by russ on 10/30/21.
//

#ifndef ASSIGNMENT_3_SCENE_OBJECTS_H
#define ASSIGNMENT_3_SCENE_OBJECTS_H

#include "Material.h"

enum ObjectClass {
    cube,
    bumpyCube,
    bunny,
    quad
};

struct Scene_Object {
    ObjectClass mClass;
    glm::mat4   mTransform;
    glm::vec3   mMinPoint;
    glm::vec3   mMaxPoint;
    Material   mMaterial;
    bool        mSelected = false;

    Scene_Object() {
        mClass = cube;
        mTransform = glm::mat4(1.0f);
        mMinPoint = glm::vec3(-0.5);
        mMaxPoint = glm::vec3(0.5);
        mMaterial = Material();
    }
    Scene_Object(ObjectClass obj):mClass(obj){
        mTransform = glm::mat4(1.0f);
        mMinPoint = glm::vec3(-0.5);
        mMaxPoint = glm::vec3(0.5);
        mMaterial = Material();
    }
    Scene_Object(ObjectClass obj, glm::vec3(color), GLfloat shine):mClass(obj){
        mTransform = glm::mat4(1.0f);
        mMinPoint = glm::vec3(-0.5);
        mMaxPoint = glm::vec3(0.5);

        Material material(color, shine);
        mMaterial = material;
    }
    void setWorldMin(glm::vec3 modelMin){
        mMinPoint = mTransform * glm::vec4(modelMin,1.0f);
    }
    void setWorldMax(glm::vec3 modelMax){
        mMaxPoint = mTransform * glm::vec4(modelMax, 1.0f);
    }
    bool isHit(const glm::vec3 &worldPoint, const glm::vec3 &rayDirection, const std::vector<Mesh>& meshes) const {
        switch(this->mClass){
            case cube:
                return meshes[0].isHit(worldPoint, rayDirection, mTransform);
            case bumpyCube:
                return meshes[1].isHit(worldPoint,rayDirection, mTransform);
            case bunny:
                return meshes[2].isHit(worldPoint,rayDirection, mTransform);
            default: return false;
        }
    }
};



#endif //ASSIGNMENT_3_SCENE_OBJECTS_H
