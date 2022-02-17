//
// Created by russ on 10/28/21.
//

#ifndef ASSIGNMENT_3_CAMERA_H
#define ASSIGNMENT_3_CAMERA_H

GLfloat EPSILON = 0.1f;

enum Projection_State{
    Orthographic,
    Projection
};

struct Camera {
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 viewMatrix;

    GLint     projectionState;
    glm::mat4 projectionMatrix;

    GLfloat   cameraSpeed;
    GLuint    screenHeight;
    GLuint    screenWidth;
    GLfloat   aspectRatio;


    Camera(GLfloat aspectRatio, GLuint screenWidth, GLuint screenHeight) :
            aspectRatio(aspectRatio),
            screenWidth(screenWidth),
            screenHeight(screenHeight) {
        cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        cameraUp = worldUp;
        cameraSpeed = 0.2;
        cameraFront = glm::vec3(0.0f);

        constructCameraFrame();

        projectionState = Projection;
        projectionMatrix = glm::perspective(glm::radians(45.0f),
                                            aspectRatio,
                                            1.0f,
                                            100.0f);
        viewMatrix = glm::mat4(1.0f);
        updateView();
    }

    void updateAspectRatio(int width, int height){
        screenHeight = height;
        screenWidth = width;
        aspectRatio = static_cast<float>(width)/static_cast<float>(height);
        updateProjection();
    }

    void updateProjState(){
        projectionState == Orthographic ?
                projectionState = Projection :
                projectionState = Orthographic;
        updateProjection();
    }

    void updateProjection(){
        switch(projectionState){
            case Orthographic:
                // Implementation for aspect ratio correction inspired by
                // https://stackoverflow.com/questions/35810782/opengl-view-projections-and-orthographic-aspect-ratio?rq=1
                projectionMatrix = glm::ortho(-aspectRatio * aspectRatio/2,
                                              aspectRatio * aspectRatio/2,
                                              -aspectRatio /2,
                                              aspectRatio /2,
                                              1.0f, 100.0f);
                break;
            case Projection:
                projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                    aspectRatio,
                                                    1.0f,
                                                    100.0f);
                break;
            default:
                break;
        }
    }

    void constructCameraFrame(){
        cameraRight = glm::normalize(glm::cross(cameraPos, worldUp));
        cameraUp = glm::normalize(glm::cross(cameraRight, cameraPos));
    }

    void updateView(){
        viewMatrix = glm::lookAt(cameraPos, cameraFront, cameraUp);
    }

    void moveRight(){
        cameraPos += cameraRight * cameraSpeed;
        constructCameraFrame();
        updateView();
    }
    void moveLeft(){
        cameraPos -= cameraRight * cameraSpeed;
        constructCameraFrame();
        updateView();
    }
    void moveUp(){
        cameraPos += cameraUp * cameraSpeed;
        constructCameraFrame();
        updateView();
        inversion();
    }
    void moveDown(){
        cameraPos -= cameraUp * cameraSpeed;
        constructCameraFrame();
        updateView();
        inversion();
    }
    void zoomIn(){
        cameraPos += (cameraFront - cameraPos) * cameraSpeed;
        constructCameraFrame();
        updateView();
    }
    void zoomOut(){
        cameraPos += (cameraFront + cameraPos) * cameraSpeed;
        constructCameraFrame();
        updateView();
    }
    void resetCamera(){
        cameraPos = glm::vec3(0.0f,0.0f,3.0f);
        constructCameraFrame();
        updateView();
        updateProjection();
    }

    glm::vec3 getWorldCoordinates(glm::vec2 &screenCoords) const{
        // Convert screen to world
        GLfloat xWorld = (screenCoords.x / ((float)screenWidth) * 2) - 1;
        GLfloat yWorld = (((float)screenHeight - screenCoords.y - 1) / ((float)screenHeight) * 2) - 1;
        // Apply inverse transform matrices
        glm::vec4 realWorld = glm::inverse(projectionMatrix * viewMatrix) * glm::vec4(xWorld, yWorld, 0.0f, 1.0);
        // Return x and y (z is taken from the camera's current position.
        return {realWorld.x, realWorld.y, realWorld.z};
    }

    void inversion(){
        if(glm::abs(cameraPos.z) < EPSILON)
            worldUp *= -1;
    }
};

#endif //ASSIGNMENT_3_CAMERA_H
