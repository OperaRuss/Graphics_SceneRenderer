//
// Created by russ on 10/28/21.
// This code is based heavily on Learn OpenGL, a tutorial by Joey DeVries.
// As the saying goes "if it ain't broke, don't fix it." That said,
// I broke this several times on accident, so there was still a lot of learning!
//

#ifndef ASSIGNMENT_3_MESH_H
#define ASSIGNMENT_3_MESH_H

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <vector>
#include "Shader.h"
#include <fstream>
#include <sstream>

struct Vertex{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;

    Vertex(glm::vec3 pos, glm::vec3 color) : position(pos), color(color){}
};

class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices);
    Mesh(const std::string &filePath);
    void draw(Shader& shader);
    glm::vec3 getMinPoint() { return minPoint; }
    glm::vec3 getMaxPoint() { return maxPoint; }
    int getNumVerts() const { return numVerts; }
    void setTexCoords(int vertIndex, glm::vec2 texCoord);
    void terminate();
    bool isHit(const glm::vec3& raySource, const glm::vec3& rayDir, const glm::mat4& modelMatrix) const;
private:
    GLuint VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<glm::vec3> normals;
    GLint numVerts;
    GLint numFaces;
    GLint numEdges;
    glm::vec3 minPoint;
    glm::vec3 maxPoint;

    void setupMesh();
    void fromFile(const std::string &file_name);
    bool getNextLine(std::ifstream &infile, std::string &line);
    void boundingCheck(const glm::vec3 &point);
    void normalizeCoordinates();
    void normalizeColors();
    void calculateNormals();
};


#endif //ASSIGNMENT_3_MESH_H
