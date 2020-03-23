#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "global.hpp"
#include "wireframe.hpp"
#include "rasteriser.hpp"

using namespace std;
using namespace glm;

// Raytracing Stuff
vec3 computeRayDirection(float x, float y);
RayTriangleIntersection getClosestIntersection(vec3 cameraPos, vec3 rayDirection, vector<ModelTriangle> triangles);
void drawRaytraced(vector<ModelTriangle> triangles);
Colour getAverageColour(vector<Colour> finalColours);
void drawRaytraceAntiAlias(vector<ModelTriangle> triangles);

// Lighting
float computeDotProduct(vec3 point, ModelTriangle t);
float calculateBrightness(int x, int y);

// Shadows
bool checkShadow(vec3 point, vector<ModelTriangle> triangles, int triangleIndex);

// Gouraud Shading
vector<pair<size_t, ModelTriangle>> getTrianglesWithVertex(vec3 point, vector<ModelTriangle> triangles);
vec3 getAverageSurfaceNormal(vector<pair<size_t, ModelTriangle>> commonTriangles);
void updateVertexNormals(vector<ModelTriangle> triangles);

float computeDotProduct(vec3 point, ModelTriangle t)
{ 
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));
  vec3 pointToLight = glm::normalize(lightSource - point);

  float dotProduct = std::max(0.0f, (float) glm::dot(surfaceNormal, pointToLight));

  return dotProduct;
}

float calculateBrightness(vec3 point, ModelTriangle t)
{ 
  vec3 pointToLight = lightSource - point;
  float distance = glm::length(pointToLight);
  float dotProduct = computeDotProduct(point, t);

  float brightness = INTENSITY * dotProduct / (FRACTION_VAL * M_PI * distance * distance);

  if(brightness < (float) AMBIENCE)
  {
    brightness = (float) AMBIENCE;
  }
  if(brightness > 1.0f)
  {
    brightness = 1.0f;
  }

  return brightness;
}

vec3 computeRayDirection(float x, float y)
{
  vec3 rayDirection = glm::normalize((vec3((x - WIDTH/2), (-(y - HEIGHT/2)), FOCAL_LENGTH) - cameraPos) * cameraOrientation);
  return rayDirection;
}

bool checkShadow(vec3 point, vector<ModelTriangle> triangles, size_t triangleIndex) 
{
  vec3 rayShadow = glm::normalize(lightSource - point);
  float distanceFromLight = glm::length(rayShadow);

  bool isShadow = false;

  for(size_t i = 0; i < triangles.size(); i++)
  {
    ModelTriangle curr = triangles.at(i);
    vec3 e0 = curr.vertices[1] - curr.vertices[0];
    vec3 e1 = curr.vertices[2] - curr.vertices[0];
    vec3 SPVector = point - curr.vertices[0];
    mat3 DEMatrix(-rayShadow, e0, e1);

    vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;

    if(inRange(u, 0.0, 1.0) && inRange(v, 0.0, 1.0) && (u+v <= 1.0) && (i != triangleIndex))
    {
      if(t < distanceFromLight && (abs(t - distanceFromLight) > (float) SHADOW_THRESH))
      {
        isShadow = true;
        break;
      }
    }
  }
  return isShadow;
}

RayTriangleIntersection getClosestIntersection(vec3 cameraPos, vec3 rayDirection, vector<ModelTriangle> triangles)
{ 
  RayTriangleIntersection result;
  result.distanceFromCamera = INFINITY;

  for(size_t i = 0; i < triangles.size(); i++)
  {
    ModelTriangle curr = triangles.at(i);
    vec3 e0 = curr.vertices[1] - curr.vertices[0];
    vec3 e1 = curr.vertices[2] - curr.vertices[0];
    vec3 SPVector = cameraPos - curr.vertices[0];
    mat3 DEMatrix(-rayDirection, e0, e1);

    vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;

    if(inRange(u, 0.0, 1.0) && inRange(v, 0.0, 1.0) && (u+v <= 1.0))
    {
      if(t < result.distanceFromCamera)
      {
        vec3 point = curr.vertices[0] + (u * e0) + (v * e1);
        float brightness = calculateBrightness(point, curr);

        bool shadow = checkShadow(point, triangles, i);

        if(shadowMode)
        {
          if(shadow)
          {
            brightness = 0.15f;
          }
        }

        curr.colour = Colour(curr.colour.red, curr.colour.green, curr.colour.blue, brightness);

        result = RayTriangleIntersection(point, t, curr);
      }
    }
  }
  if(result.distanceFromCamera == INFINITY)
  {
    result.distanceFromCamera = -INFINITY;
  }
  return result;
}

void drawRaytraced(vector<ModelTriangle> triangles)
{ 
  for(int y = 0; y < HEIGHT; y++)
  {
    for(int x = 0; x < WIDTH; x++)
    {
      vec3 ray = computeRayDirection((float) x, (float) y);
      RayTriangleIntersection closestIntersect = getClosestIntersection(cameraPos, ray, triangles);
      if(closestIntersect.distanceFromCamera != -INFINITY)
      {
        window.setPixelColour(x, y, closestIntersect.intersectedTriangle.colour.packWithBrightness());
      }
    }
  }
  cout << "RAYTRACING DONE" << endl;
}

Colour getAverageColour(vector<Colour> finalColours)
{
  Colour average = Colour(finalColours.at(0).red * 2, finalColours.at(0).green * 2, finalColours.at(0).blue * 2, finalColours.at(0).brightness * 2);
  for(size_t i = 1; i < finalColours.size(); i++)
  {
    int red = average.red + finalColours.at(i).red;
    int green = average.green + finalColours.at(i).green;
    int blue = average.blue + finalColours.at(i).blue;
    float brightness = average.brightness + finalColours.at(i).brightness;

    average = Colour(red, green, blue, brightness);
  }
  int denom = finalColours.size() + 2;
  average = Colour(average.red/denom, average.green/denom, average.blue/denom, average.brightness/denom);
  Colour toReturn = Colour(average.red * average.brightness, average.green * average.brightness, average.blue * average.brightness);
  return toReturn;
}

void drawRaytraceAntiAlias(vector<ModelTriangle> triangles)
{
  vector<vec2> quincunx;
  quincunx.push_back(vec2(0.0f, 0.0f));
  quincunx.push_back(vec2(0.5f, 0.0f));
  quincunx.push_back(vec2(-0.5f, 0.0f));
  quincunx.push_back(vec2(0.0f, 0.5f));
  quincunx.push_back(vec2(0.0f, -0.5f));

  for(int y = 0; y < HEIGHT; y++)
  {
    for(int x = 0; x < WIDTH; x++)
    {
      vector<Colour> finalColours;

      for(size_t i = 0; i < quincunx.size(); i++)
      {
        vec3 ray = computeRayDirection(x + quincunx.at(i).x, y + quincunx.at(i).y);
        RayTriangleIntersection closestIntersect = getClosestIntersection(cameraPos, ray, triangles);
        if(closestIntersect.distanceFromCamera != -INFINITY)
        {
          finalColours.push_back(closestIntersect.intersectedTriangle.colour);
        }
      }
      if(finalColours.size() > 0)
      {
        Colour c = getAverageColour(finalColours);
        window.setPixelColour(x, y, c.pack());
      }
    }
  }
  cout << "RAYTRACE ANTI-ALIAS DONE" << endl;
}

vector<pair<size_t, ModelTriangle>> getTrianglesWithVertex(vec3 point, vector<ModelTriangle> triangles)
{
  vector<pair<size_t, ModelTriangle>> result;

  for(size_t i = 0; i < triangles.size(); i++)
  {
    ModelTriangle t = triangles.at(i);
    if(point == t.vertices[0]) result.push_back(make_pair(0, t));
    if(point == t.vertices[1]) result.push_back(make_pair(1, t));
    if(point == t.vertices[2]) result.push_back(make_pair(2, t));
  }
  return result;
}

vec3 getAverageSurfaceNormal(vector<pair<size_t, ModelTriangle>> commonTriangles)
{
  vec3 average = vec3(0, 0, 0);
  for(size_t i = 0; i < commonTriangles.size(); i++)
  {
    ModelTriangle t = commonTriangles.at(i).second;
    vec3 diff1 = t.vertices[1] - t.vertices[0];
    vec3 diff2 = t.vertices[2] - t.vertices[0];

    vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));

    average += surfaceNormal;
  }
  average = average/((float)commonTriangles.size());
  return average;
}

void updateVertexNormals(vector<ModelTriangle> triangles)
{
  for(size_t i = 0; i < triangles.size(); i++)
  {
    ModelTriangle t = triangles.at(i);
    for(int j = 0; j < 3; j++)
    {
      vec3 vertex = t.vertices[j];
      if(t.vertexNormals[j] != vec3(1, 1, 1))
      {
        vector<pair<size_t, ModelTriangle>> commonTriangles = getTrianglesWithVertex(vertex, triangles);
        vec3 averageNormal = getAverageSurfaceNormal(commonTriangles);
        for(pair<size_t, ModelTriangle> common : commonTriangles)
        {
          ModelTriangle curr = common.second;
          curr.vertexNormals[common.first] = averageNormal;
        }
      }
    }
  }
}

#endif