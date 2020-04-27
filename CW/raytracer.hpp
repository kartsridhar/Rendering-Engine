#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "global.hpp"
#include "wireframe.hpp"
#include "rasteriser.hpp"

using namespace std;
using namespace glm;

// Raytracing Stuff
vec3 computeRayDirection(float x, float y);
RayTriangleIntersection getClosestIntersection(vec3 cameraPos, vec3 rayDirection, vector<ModelTriangle> triangles, int depth, float ior);
void drawRaytraced(vector<ModelTriangle> triangles);
Colour getAverageColour(vector<Colour> finalColours);
void drawRaytraceAntiAlias(vector<ModelTriangle> triangles);

// Lighting
float computeDotProduct(vec3 point, vec3 surfaceNormal, ModelTriangle t);
float calculateBrightness(vec3 point, ModelTriangle t, vec3 rayDirection, vector<ModelTriangle> triangles);

// Shadows
bool checkHardShadow(vec3 point, vec3 rayShadow, vector<ModelTriangle> alteredTriangles);
float checkSoftShadow(vec3 point, vector<ModelTriangle> alteredTriangles);

// Smooth Shading
vector<float> calculateVertexBrightness(vector<ModelTriangle> triangles, ModelTriangle t, vec3 rayDirection);
float calculateGouraudBrightness(vector<ModelTriangle> triangles, vec3 point, ModelTriangle t, float u, float v, vec3 rayDirection);
float calculatePhongBrightness(vector<ModelTriangle> triangles, vec3 point, ModelTriangle t, float u, float v, vec3 rayDirection);

// Bump Mapping
float calculateBumpBrightness(vec3 point, ModelTriangle t, vec3 rayDirection, vector<ModelTriangle> triangles, vec3 triangleNormal);

// Reflection/Mirror
vec3 computeReflectedRay(vec3 incidentRay, ModelTriangle t);
RayTriangleIntersection getFinalIntersection(vector <ModelTriangle> triangles, vec3 ray,vec3 origin,RayTriangleIntersection* intersection,int depth);
// Refraction/Glass
vec3 computeInternalReflectedRay(vec3 incidentRay, ModelTriangle t);
vec4 refract(vec3 incidentRay, vec3 surfaceNormal, float ior);
float fresnel(vec3 incidentRay, vec3 surfaceNormal, float ior);
Colour calculateGlassColour(vector<ModelTriangle> triangles, vec3 rayDirection, vec3 intersectionPoint, ModelTriangle t, int depth);

float calculateBrightness(vec3 point, ModelTriangle t, vec3 rayDirection, vector<ModelTriangle> triangles)
{ 
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));

  vec3 pointToLight = lightSource - point;
  float distance = glm::length(pointToLight);

  float brightness = INTENSITY / (FRACTION_VAL * M_PI * distance * distance);

  float dotProduct = std::max(0.0f, (float) glm::dot(surfaceNormal, glm::normalize(pointToLight)));
  brightness *= pow(dotProduct, 1.0f);

  // Specular Highlighting
  if(reflectiveMode)
  {
    vec3 flipped = -1.0f * rayDirection;
    vec3 reflected = pointToLight - (2.0f * point * glm::dot(pointToLight, point));
    float angle = std::max(0.0f, glm::dot(glm::normalize(flipped), glm::normalize(reflected)));
    brightness += pow(angle, 1.0f);
  }

  if(brightness < (float) AMBIENCE)
  {
    brightness = (float) AMBIENCE;
  }

  if(hardShadowMode)
  {
    vector<ModelTriangle> alteredTriangles = removeIntersectedTriangle(triangles, t);
    bool hardShadow = checkHardShadow(point, pointToLight, alteredTriangles);
    if(hardShadow)
      brightness = 0.15f;
  } 

  if(softShadowMode)
  {
    vector<ModelTriangle> alteredTriangles = removeIntersectedTriangle(triangles, t);
    float penumbra = checkSoftShadow(point, alteredTriangles);
    brightness *= (1 - penumbra);
    if(brightness < 0.15f) brightness = 0.15f;
  }

  if(brightness > 1.0f)
  {
    brightness = 1.0f;
  }

  return brightness;
}

vector<float> calculateVertexBrightness(vector<ModelTriangle> triangles, ModelTriangle t, vec3 rayDirection)
{
  vector<vec3> vertexNormals = getTriangleVertexNormals(t, triangleVertexNormals);
  vector<float> brightnessVals;
  for(int i = 0; i < 3; i++)
  {
    vec3 vertex = vertexNormals[i];
    vec3 vertexToLight = lightSource - t.vertices[i];
    float distance = glm::length(vertexToLight);

    float brightness = INTENSITY / (FRACTION_VAL * M_PI * distance * distance);

    float dotProduct = std::max(0.0f, (float) glm::dot(vertex, glm::normalize(vertexToLight)));
    brightness *= pow(dotProduct, 1.0f);

    // Specular Highlighting
    if(reflectiveMode)
    {
      vec3 flipped = -1.0f * rayDirection;
      vec3 reflected = vertexToLight - (2.0f * vertex * glm::dot(vertexToLight, vertex));
      float angle = std::max(0.0f, glm::dot(glm::normalize(flipped), glm::normalize(reflected)));
      brightness += pow(angle, 1.0f);
    }

    if(brightness < (float) AMBIENCE)
    {
      brightness = (float) AMBIENCE;
    }

    if(brightness > 1.0f)
    {
      brightness = 1.0f;
    }

    brightnessVals.push_back(brightness);
  }
  return brightnessVals;
}

float calculateGouraudBrightness(vector<ModelTriangle> triangles, vec3 point, ModelTriangle t, float u, float v, vec3 rayDirection)
{
  vector<float> brightnessVals = calculateVertexBrightness(triangles, t, rayDirection);
  float b0 = brightnessVals[1] - brightnessVals[0];
  float b1 = brightnessVals[2] - brightnessVals[0];

  float result = brightnessVals[0] + (u * b0) + (v * b1);

  return result;
}

float calculatePhongBrightness(vector<ModelTriangle> triangles, vec3 point, ModelTriangle t, float u, float v, vec3 rayDirection)
{
  vector<vec3> vertexNormals = getTriangleVertexNormals(t, triangleVertexNormals);
  vec3 n0 = vertexNormals[1] - vertexNormals[0];
  vec3 n1 = vertexNormals[2] - vertexNormals[0];

  vec3 adjustedNormal = vertexNormals[0] + (u * n0) + (v * n1);

  vec3 pointToLight = lightSource - point;
  float distance = glm::length(pointToLight);

  float brightness = INTENSITY / (FRACTION_VAL * M_PI * distance * distance);

  float dotProduct = std::max(0.0f, (float) glm::dot(adjustedNormal, glm::normalize(pointToLight)));
  brightness *= pow(dotProduct, 1.0f);

  // Specular Highlighting
  if(reflectiveMode)
  {
    vec3 flipped = -1.0f * rayDirection;
    vec3 reflected = pointToLight - (2.0f * point * glm::dot(pointToLight, point));
    float angle = std::max(0.0f, glm::dot(glm::normalize(flipped), glm::normalize(reflected)));
    brightness += pow(angle, 1.0f);
  }

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

float calculateBumpBrightness(vec3 point, ModelTriangle t, vec3 rayDirection, vector<ModelTriangle> triangles, vec3 triangleNormal)
{
  vec3 pointToLight = lightSource - point;
  float distance = glm::length(pointToLight);

  float brightness = INTENSITY / (FRACTION_VAL * M_PI * distance * distance);

  float dotProduct = std::max(0.0f, (float) glm::dot(triangleNormal, glm::normalize(pointToLight)));
  brightness *= pow(dotProduct, 1.0f);

  // Specular Highlighting
  if(reflectiveMode)
  {
    vec3 flipped = -1.0f * rayDirection;
    vec3 reflected = pointToLight - (2.0f * point * glm::dot(pointToLight, point));
    float angle = std::max(0.0f, glm::dot(glm::normalize(flipped), glm::normalize(reflected)));
    brightness += pow(angle, 1.0f);
  }

  if(brightness < (float) AMBIENCE)
  {
    brightness = (float) AMBIENCE;
  }

  if(hardShadowMode)
  {
    vector<ModelTriangle> alteredTriangles = removeIntersectedTriangle(triangles, t);
    bool hardShadow = checkHardShadow(point, pointToLight, alteredTriangles);
    if(hardShadow)
      brightness = 0.15f;
  } 

  if(softShadowMode)
  {
    vector<ModelTriangle> alteredTriangles = removeIntersectedTriangle(triangles, t);
    float penumbra = checkSoftShadow(point, alteredTriangles);
    brightness *= (1 - penumbra);
    if(brightness < 0.15f) brightness = 0.15f;
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

bool checkHardShadow(vec3 point, vec3 rayShadow, vector<ModelTriangle> alteredTriangles) 
{
  float distanceFromLight = glm::length(rayShadow);
  
  bool isShadow = false;

  for(size_t i = 0; i < alteredTriangles.size(); i++)
  {
    ModelTriangle curr = alteredTriangles.at(i);
    vec3 e0 = curr.vertices[1] - curr.vertices[0];
    vec3 e1 = curr.vertices[2] - curr.vertices[0];
    vec3 SPVector = point - curr.vertices[0];
    mat3 DEMatrix(-glm::normalize(rayShadow), e0, e1);

    vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;

    if(inRange(u, 0.0, 1.0) && inRange(v, 0.0, 1.0) && (u+v <= 1.0))
    {
      if(t < distanceFromLight && t > 0.0f)
      {
        isShadow = true;
        break;
      }
    }
  }
  return isShadow;
}

float checkSoftShadow(vec3 point, vector<ModelTriangle> alteredTriangles)
{
  float penumbra = 0.0f;
  int count = 0;
  for(size_t i = 0; i < lightSources.size(); i++)
  {
    vec3 rayShadow = lightSources.at(i) - point;
    bool shadow = checkHardShadow(point, rayShadow, alteredTriangles);
    if(shadow) count += 1;
  }
  penumbra = count / (float) lightSources.size();
  return penumbra;
}

RayTriangleIntersection getClosestIntersection(vec3 cameraPos, vec3 rayDirection, vector<ModelTriangle> triangles, int depth, float ior)
{ 
  RayTriangleIntersection result;
  result.distanceFromCamera = INFINITY;

  #pragma omp parallel
  #pragma omp for
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
        float brightness = calculateBrightness(point, curr, rayDirection, triangles);

        uint32_t intersection_col = 0;
        if (curr.tag == "checker"){
          vec2 e0_tex = curr.texturepoints[1] * 738.f - curr.texturepoints[0] * 738.f;
          vec2 e1_tex = curr.texturepoints[2] * 738.f - curr.texturepoints[0] * 738.f;
          vec2 tex_point_final = curr.texturepoints[0] * 738.f + (u * e0_tex) + (v * e1_tex);
          intersection_col=checkcols[round(tex_point_final.x) + round(tex_point_final.y) * 738];
        }
        else if (curr.tag == "hackspace"){
          vec2 e0_tex = curr.texturepoints[1] * 300.f - curr.texturepoints[0] * 300.f;
          vec2 e1_tex = curr.texturepoints[2] * 300.f - curr.texturepoints[0] * 300.f;
          vec2 tex_point_final = curr.texturepoints[0] * 300.f + (u * e0_tex) + (v * e1_tex);
          if (int(tex_point_final.x)<=300 && int(tex_point_final.x)>=0 && int(tex_point_final.y)<=300 && int(tex_point_final.y)>=0)
            intersection_col=pixelColours[(int(tex_point_final.x)-1) + (int(tex_point_final.y)-1) * 300];
        }
        
        Colour colour = Colour();
        vec3 newColour;

        if(curr.tag == "cornell")
        {
          if(reflectiveMode && curr.colour.reflectivity && depth > 0)
          {
            vector<ModelTriangle> reflectionTriangles = removeIntersectedTriangle(triangles, curr);
            vec3 incidentRay = glm::normalize(point - cameraPos);
            vec3 reflectedRay = computeReflectedRay(incidentRay, curr);
            RayTriangleIntersection mirrorIntersect = getClosestIntersection(point, reflectedRay, reflectionTriangles, depth - 1, ior);
            if(mirrorIntersect.distanceFromCamera >= INFINITY) colour = Colour(0, 0, 0);
            else colour = mirrorIntersect.colour;
          }
          else if(refractiveMode && curr.colour.refractivity && depth > 0 && depth <= 5)
          {
            // colour = calculateGlassColour(triangles, rayDirection, point, curr, depth - 1);
            RayTriangleIntersection final_intersection = getFinalIntersection(triangles,rayDirection,cameraPos,nullptr,1);
            colour = final_intersection.colour;
          }
          else
          {
            newColour = vec3(curr.colour.red, curr.colour.green, curr.colour.blue);
            colour = Colour(newColour.x, newColour.y, newColour.z);
            colour.brightness = brightness;
          }
        }
        else if(curr.tag == "hackspace" || curr.tag == "checker")
        { 
          newColour = unpackColour(intersection_col);
          colour = Colour(newColour.x, newColour.y, newColour.z);
          colour.brightness = brightness;
        }
        else if(curr.tag == "sphere")
        {
          if(gouraudMode)
          {
            newColour = vec3(curr.colour.red, curr.colour.green, curr.colour.blue);
            brightness = calculateGouraudBrightness(triangles, point, curr, u, v, rayDirection);
            colour = Colour(newColour.x, newColour.y, newColour.z);
            colour.brightness = brightness;
          }
          else if(phongMode)
          {
            newColour = vec3(curr.colour.red, curr.colour.green, curr.colour.blue);
            brightness = calculatePhongBrightness(triangles, point, curr, u, v, rayDirection);
            colour = Colour(newColour.x, newColour.y, newColour.z);
            colour.brightness = brightness;
          }
          else
          {
            newColour = vec3(curr.colour.red, curr.colour.green, curr.colour.blue);
            colour = Colour(newColour.x, newColour.y, newColour.z);
            colour.brightness = brightness;
          }    
        }
        else if(curr.tag == "bump") 
        {
          newColour = vec3(curr.colour.red, curr.colour.green, curr.colour.blue);
          colour = Colour(newColour.x, newColour.y, newColour.z);
          
          vec2 e0_bump = curr.texturepoints[1] * 949.f - curr.texturepoints[0] * 949.f;
          vec2 e1_bump = curr.texturepoints[2] * 949.f - curr.texturepoints[0] * 949.f;
          vec2 bump_point = curr.texturepoints[0] * 949.f + (u * e0_bump) + (v * e1_bump);

          if(int(bump_point.x) >= 0 && int(bump_point.x) <= 949 && int(bump_point.y) >= 0 && int(bump_point.y) <= 949)
          {
            vec3 bump_point_normal = bumpNormals[(int(bump_point.x)) + (int(bump_point.y)) * 949];
            brightness = calculateBumpBrightness(point, curr, rayDirection, triangles, bump_point_normal);
            colour.brightness = brightness;
          }
        }
        result = RayTriangleIntersection(point, t, curr, colour);
      }
    }
  }
  return result;
}

void drawRaytraced(vector<ModelTriangle> triangles)
{ 
  #pragma omp parallel
  #pragma omp for
  for(int y = 0; y < HEIGHT; y++)
  {
    for(int x = 0; x < WIDTH; x++)
    {
      vec3 ray = computeRayDirection((float) x, (float) y);
      RayTriangleIntersection closestIntersect = getClosestIntersection(cameraPos, ray, triangles, 5, 1.0f);
      
      if(closestIntersect.distanceFromCamera < INFINITY)
      {
        window.setPixelColour(x, y, closestIntersect.colour.packWithBrightness());
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
  Colour toReturn = Colour(average.red, average.green, average.blue, average.brightness);
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

  #pragma omp parallel
  #pragma omp for
  for(int y = 0; y < HEIGHT; y++)
  {
    for(int x = 0; x < WIDTH; x++)
    {
      vector<Colour> finalColours;

      for(size_t i = 0; i < quincunx.size(); i++)
      {
        vec3 ray = computeRayDirection(x + quincunx.at(i).x, y + quincunx.at(i).y);
        RayTriangleIntersection closestIntersect = getClosestIntersection(cameraPos, ray, triangles, 1, 1.f);

        if(closestIntersect.distanceFromCamera < INFINITY)
        {
          finalColours.push_back(closestIntersect.colour);
        }
      }
      if(finalColours.size() > 0)
      {
        Colour c = getAverageColour(finalColours);
        window.setPixelColour(x, y, c.packWithBrightness());
      }
    }
  }
  cout << "RAYTRACE ANTI-ALIAS DONE" << endl;
}

// Reflection/Mirror stuff
vec3 computeReflectedRay(vec3 incidentRay, ModelTriangle t)
{
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));

  vec3 reflected = incidentRay - (2.0f * surfaceNormal * glm::dot(incidentRay, surfaceNormal));
  return reflected;
}

vec3 computeInternalReflectedRay(vec3 incidentRay, ModelTriangle t)
{
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));
  
  vec3 reflected = glm::normalize(incidentRay - 2.f * surfaceNormal*(glm::dot(surfaceNormal, incidentRay)));
  return reflected;
}

// Refraction/Glass stuff
// vec3 refract(vec3 incidentRay, vec3 surfaceNormal, float ior)
// {
//   float cosi = clamp(-1.0f, 1.0f, glm::dot(incidentRay, surfaceNormal)); 
//   float etai = 1, etat = ior;

//   if(cosi < 0.f)
//   {
//     cosi = -cosi;
//   }
//   else
//   {
//     std::swap(etai, etat);
//     surfaceNormal = -surfaceNormal;
//   }
//   float ratio = etai / etat;
//   float k = 1 - ratio * ratio * (1 - cosi * cosi);

//   vec3 refracted = ratio * incidentRay + (ratio * cosi - round(sqrtf(k))) * surfaceNormal;

//   if(k < 0) return vec3(0, 0, 0); else return refracted;
// }

vec4 refract(vec3 incidentRay, vec3 surfaceNormal, float ior)
{
  float cosi = clamp(-1.0f, 1.0f, glm::dot(incidentRay, surfaceNormal)); 
  float etai = 1, etat = ior;
  float ratio = etai / etat;

  if(cosi > 0) 
  {
    ratio = 1 / ratio;
    surfaceNormal = -surfaceNormal;
  }
  
  float cost = abs(cosi);

  float k = 1 - ratio * ratio * (1 - cost * cost);
  if(k < 0) return vec4(0, 0, 0, 0);

  vec3 refracted = glm::normalize(ratio * incidentRay + (ratio * cost - sqrtf(k)) * surfaceNormal);
  return vec4(refracted.x, refracted.y, refracted.z, sign(cosi));
}

float fresnel(vec3 incidentRay, vec3 surfaceNormal, float ior)
{
  float cosi = clamp(-1.0f, 1.0f, glm::dot(incidentRay, surfaceNormal)); 
  float etai = 1;
  float etat = ior;

  if(cosi > 0) std::swap(etai, etat);

  float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
  float kr;
  if(sint >= 1) kr = 1;
  else
  {
    float cost = sqrtf(std::max(0.0f, (1 - sint * sint)));
    cosi = fabsf(cosi);
    float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
    float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    kr = (Rs * Rs + Rp * Rp) / 2;
  }
  return kr;
}

Colour calculateGlassColour(vector<ModelTriangle> triangles, vec3 rayDirection, vec3 intersectionPoint, ModelTriangle t, int depth)
{ 
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));

  vector<ModelTriangle> filteredTriangles = removeIntersectedTriangle(triangles, t);

  // Do reflection
  vec3 reflectedRay = computeInternalReflectedRay(rayDirection, t);
  RayTriangleIntersection reflectionIntersect = getClosestIntersection(intersectionPoint, reflectedRay, filteredTriangles, depth - 1, 1.f);

  // Do refraction
  vec4 refraction = refract(rayDirection, surfaceNormal, 1.3f);
  vec3 refractedRay = vec3(refraction[0], refraction[1], refraction[2]);
  if(refractedRay == vec3(0, 0, 0)) return reflectionIntersect.colour;

  if(refraction[3] == 1) refractedRay = -1.0f * refractedRay;

  RayTriangleIntersection refractionIntersect = getClosestIntersection(intersectionPoint, refractedRay, filteredTriangles, depth - 1, 1.5f);
  return refractionIntersect.colour;
}

vec3 computenorm(ModelTriangle t){
  vec3 diff1 = t.vertices[1] - t.vertices[0];
  vec3 diff2 = t.vertices[2] - t.vertices[0];

  vec3 surfaceNormal = glm::normalize(glm::cross(diff1, diff2));
  return surfaceNormal;
}

RayTriangleIntersection getIntersection(vec3 ray,ModelTriangle triangle,vec3 origin){
  vec3 e0 = triangle.vertices[1]-triangle.vertices[0];
  vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
  vec3 SPVector = origin - triangle.vertices[0];
  vec3 negRay = -ray;
  glm::mat3 DEMatrix(negRay,e0,e1);
  vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
  vec3 point = origin+ray*possibleSolution.x;
  RayTriangleIntersection r = RayTriangleIntersection(point,possibleSolution.x,triangle,triangle.colour);
  return r;

}

vec3 refractRay(vec3 ray,vec3 norm,float refractive_index){
  float cosi = glm::dot(ray,norm);
  if (cosi < -1) cosi = -1;
  else if (cosi<-1)cosi=-1;
  float etai=1;
  float etat = refractive_index;
  vec3 n = norm;
  if (cosi < 0){cosi = -cosi;}
  else{std::swap(etai,etat);n=-norm;}
  float eta = etai/etat;
  float k = 1 - eta * eta * (1-cosi * cosi);
  return k < 0 ? vec3(0,0,0) : eta * ray + (eta*cosi-sqrt(k))*n;
}

vec3 calcReflectedRay(vec3 ray, ModelTriangle t){
  vec3 incidence = ray;
  vec3 norm = computenorm(t);
  vec3 reflect = incidence = 2.f * (norm * (glm::dot(incidence,norm)));
  reflect = glm::normalize(reflect);
  return reflect;
}

bool isEqualTriangle(ModelTriangle t1, ModelTriangle t){
  return (t1.vertices == t.vertices);
}

RayTriangleIntersection getFinalIntersection(vector <ModelTriangle> triangles, vec3 ray,vec3 origin,RayTriangleIntersection* original_intersection,int depth){
  RayTriangleIntersection final_intersection;
  final_intersection.distanceFromCamera = INFINITY;
  final_intersection.intersectedTriangle.colour=Colour(0,0,0);
  vec3 newColour;
    // vector<ModelTriangle> filteredTriangles = removeIntersectedTriangle(triangles, t);
  // vector<ModelTriangle> filteredTriangles = triangles;
  if (depth<5){
    float minDist = INFINITY;
    for (size_t i=0;i<triangles.size();i++){
      if (triangles.at(i).colour.refractivity){
        RayTriangleIntersection intersection = getIntersection(ray,triangles.at(i),origin);
        float distance = intersection.distanceFromCamera;
        bool hit =  (original_intersection != nullptr && !isEqualTriangle(original_intersection->intersectedTriangle,triangles.at(i)))|| original_intersection == nullptr;
        if (distance < minDist && hit){
          final_intersection = intersection;
          minDist = distance;
        }
      }
    }
    if (final_intersection.distanceFromCamera != INFINITY){
      ModelTriangle t = final_intersection.intersectedTriangle;
      vec3 point = final_intersection.intersectionPoint;
      if (t.colour.refractivity){
        vec3 norm = computenorm(t);
        vec3 glassReflectedRay = calcReflectedRay(ray,t);
        RayTriangleIntersection glass_reflected_intersection = getFinalIntersection(triangles,glassReflectedRay,point,&final_intersection,depth+1);
        Colour r = glass_reflected_intersection.intersectedTriangle.colour;
        vec3 reflected_colour = vec3(r.red,r.green,r.blue);
        float refractive_index = 1.5;
        vec3 glassRefractedRay = refractRay(ray,norm,refractive_index);
        RayTriangleIntersection final_glass_intersection = getFinalIntersection(triangles,glassRefractedRay,point,&final_intersection,depth+1);

        Colour c = final_glass_intersection.intersectedTriangle.colour;
        vec3 refracted_colour = vec3(c.red,c.green,c.blue);
        vec3 fin_colour;
        float kr = fresnel(ray,norm,refractive_index);
        if (glassRefractedRay == vec3(0,0,0)) fin_colour = reflected_colour;
        else{ fin_colour = reflected_colour+kr+refracted_colour*(1-kr);}
        final_intersection.intersectedTriangle.colour = Colour(fin_colour.x,fin_colour.y,fin_colour.z);
      }
    }
  }
  return final_intersection;


}


#endif
