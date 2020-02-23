#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 500
#define HEIGHT 500
// #define MTLPATH "/home/asel/Documents/ComputerGraphics/Lab3/cornell-box.mtl"
// #define OBJPATH "/home/asel/Documents/ComputerGraphics/Lab3/cornell-box.obj"
#define MTLPATH "/home/ks17226/Documents/ComputerGraphics/Lab3/cornell-box.mtl"
#define OBJPATH "/home/ks17226/Documents/ComputerGraphics/Lab3/cornell-box.obj"
// #define MTLPATH "/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box.mtl"
// #define OBJPATH "/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box.obj"

vec3 cameraPos(0, 0, 300);
mat3 cameraOrientation = mat3(1, 0, 0
                              0, 1, 0
                              0, 0, 1);

vector<ModelTriangle> readObj(float scale);
vector<Colour> readMaterial(string fname);
CanvasTriangle modelToCanvas(ModelTriangle t);
void drawLine(CanvasPoint p1, CanvasPoint p2, Colour c);
void update();
void handleEvent(SDL_Event event);
void drawFilledWireframe(vector<ModelTriangle> tris);
void drawStroke(CanvasTriangle t, Colour c);
vector<CanvasPoint> interpolate(CanvasPoint from, CanvasPoint to, int numberOfValues);
vector<float> interpolation(float from, float to, int numberOfValues);
void computeDepth(CanvasTriangle t, double *depthBuffer);
void depthBuffer(vector<ModelTriangle> tris);

Colour getColourFromName(string mat, vector<Colour> colours)
{
  Colour result = Colour(0, 0, 0);
  for(size_t i = 0; i < colours.size(); i++)
  {
    if(mat == colours[i].name)
    {
      result.red = colours[i].red;
      result.blue = colours[i].blue;
      result.green = colours[i].green;
      break;
    }
  }
  return result;
}

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

int main(int argc, char* argv[])
{
  SDL_Event event;
  vector <ModelTriangle> triangl;
  vector <Colour> colours;
  triangl = readObj(50);
  colours = readMaterial(MTLPATH);
  // drawFilledWireframe(triangl);

  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event))
    {
      handleEvent(event);
      depthBuffer(triangl);
    } 
    update();

    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
}

vector<CanvasPoint> interpolate(CanvasPoint from, CanvasPoint to, int numberOfValues)
{
  vector<CanvasPoint> vals;
  for(int i = 0; i <= numberOfValues; i++)
  {
    CanvasPoint p;
    p.x = from.x + (i * (to.x - from.x)/numberOfValues);
    p.y = from.y + (i * (to.y - from.y)/numberOfValues);
    p.texturePoint.x = from.texturePoint.x + (i * (to.texturePoint.x - from.texturePoint.x)/numberOfValues);
    p.texturePoint.y = from.texturePoint.y + (i * (to.texturePoint.y - from.texturePoint.y)/numberOfValues);
    double depth = from.depth + (i * (to.depth - from.depth)/numberOfValues);
    p.depth = depth;
    vals.push_back(p);
  }
  return vals;
}

void update()
{
  // Function for performing animation (shifting artifacts or moving the camera)
}

void drawLine(CanvasPoint p1, CanvasPoint p2, Colour c)
{
  float dx = p2.x - p1.x;
  float dy = p2.y - p1.y;
  float numberOfValues = std::max(abs(dx), abs(dy));

  float xChange = dx/(numberOfValues);
  float yChange = dy/(numberOfValues);

  for(float i = 0; i < numberOfValues; i++)
  {
    float x = p1.x + (xChange * i);
    float y = p1.y + (yChange * i);
    uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);
    window.setPixelColour(round(x), round(y), colour);
  }
}
void drawStroke(CanvasTriangle t, Colour c)
{
  drawLine(t.vertices[0], t.vertices[1], c);
  drawLine(t.vertices[1], t.vertices[2], c);
  drawLine(t.vertices[2], t.vertices[0], c);
}

void drawFilled(CanvasTriangle f, Colour c)
{
  CanvasPoint p1 = f.vertices[0];
  CanvasPoint p2 = f.vertices[1];
  CanvasPoint p3 = f.vertices[2];

  if(p1.y < p2.y)
  {
    std::swap(p1, p2);
  }
  if(p1.y < p3.y)
  {
    std::swap(p1, p3);
  }
  if(p2.y < p3.y)
  {
    std::swap(p2, p3);
  }

  // p1 = top, p2 = mid, p3 = bot

  float ratio = (p1.y - p2.y)/(p1.y - p3.y);
  CanvasPoint extraPoint;
  extraPoint.x = p1.x - ratio*(p1.x - p3.x);
  extraPoint.y = p1.y - ratio*(p1.y - p3.y);

  // Interpolation
  int numberOfValuesTop = (p1.y - p2.y);
  int numberOfValuesBot = (p2.y - p3.y);

  vector<CanvasPoint> p1_extraPoint = interpolate(p1, extraPoint, ceil(numberOfValuesTop)+1);
  vector<CanvasPoint> p1_p2 = interpolate(p1, p2, ceil(numberOfValuesTop)+1);
  vector<CanvasPoint> p3_extraPoint = interpolate(p3, extraPoint, ceil(numberOfValuesBot)+1);
  vector<CanvasPoint> p3_p2 = interpolate(p3, p2, ceil(numberOfValuesBot)+1);

  for(int i = 0; i <= numberOfValuesTop; i++)
  {
    drawLine(p1_extraPoint[i], p1_p2[i], c);
  }

  for(int i = 0; i <= numberOfValuesBot+1; i++)
  {
    drawLine(p3_extraPoint[i], p3_p2[i], c);
  }
  drawStroke(f, c);
}

vector<Colour> readMaterial(string fname)
{
  ifstream fp;
  fp.open(fname);

  vector<Colour> colours;
  while(!fp.eof())
  {
    string comment, colourInfo, newline;
    getline(fp, comment);
    string *splitName = split(comment, ' ');

    getline(fp, colourInfo);
    string *splitColourInfo = split(colourInfo, ' ');

    int r = stof(splitColourInfo[1]) * 255;
    int g = stof(splitColourInfo[2]) * 255;
    int b = stof(splitColourInfo[3]) * 255;

    Colour c = Colour(splitName[1], r, g, b);

    getline(fp, newline);
    colours.push_back(c);
  }
  fp.close();
    // cout << "finished mat"<<endl;
  return colours;
}

vector<ModelTriangle> readObj(float scale)
{
  vector <ModelTriangle> tris;
  ifstream fp;
  vector<Colour> colours = readMaterial(MTLPATH);
  vector<vec3> vertic;
  fp.open(OBJPATH);

  if(fp.fail())
    cout << "fails" << endl;

  string newline;
  getline(fp,newline);
  getline(fp,newline);

  while(!fp.eof())
  {
    string light;
    string comment;
    string mat;
    getline(fp, comment);

    if (!comment.empty())
    {
      string *splitcomment = split(comment,' ');
      if (splitcomment[0] == "usemtl")
      {
        mat = splitcomment[1];
        while(true)
        {
          getline(fp,comment);
          if (!comment.empty())
          {
            string *splitcomment = split(comment,' ');
            if (splitcomment[0]=="v")
            {
              float x = stof(splitcomment[1]) * scale;
              float y = stof(splitcomment[2]) * scale;
              float z = stof(splitcomment[3]) * scale;
              vec3 verts = vec3(x,y,z);
              vertic.push_back(verts);
            }
            else
            {
              break;
            }
          }
        }
      }
    }
  }
  // cout<<vertic.size()<<endl;
  // cout << "vertices pass done" << endl;

  fp.clear();
  fp.seekg(0,ios::beg);
  if(fp.fail())
    cout << "fails" << endl;
  // cout << "stream open" <<endl;
  getline(fp,newline);
  getline(fp,newline);

  while(!fp.eof())
  {
    // string light;
    string comment_new;
    string mat;
    getline(fp,comment_new);
    if (!comment_new.empty())
    {
      string *splitcomment = split(comment_new,' ');
      if (splitcomment[0] == "usemtl")
      {
        mat = splitcomment[1];
        Colour tricolour = getColourFromName(mat,colours);

        while(true){
          getline(fp,comment_new);
          splitcomment = split(comment_new,' ');
          if (splitcomment[0] == "f" && !comment_new.empty()){break;}
        }

      bool not_reach = true;
        while (not_reach)
        {
          // cout << "this is a comment"<<comment_new << endl;
          if (!comment_new.empty())
          {
            splitcomment = split(comment_new,' ');
            if (splitcomment[0]=="f")
            {
              int first_vert = stoi(splitcomment[1].substr(0, splitcomment[1].size()-1));
              int second_vert = stoi(splitcomment[2].substr(0, splitcomment[2].size()-1));
              int third_vert = stoi(splitcomment[3].substr(0, splitcomment[3].size()-1));

              tris.push_back(ModelTriangle(vertic[first_vert-1],vertic[second_vert-1],vertic[third_vert-1],tricolour));
            }
            else{break;}

          }
          if (!fp.eof()){
          getline(fp,comment_new);
          }
          else{not_reach=false;}

        }
      }
    }
  }
  fp.close();
// cout << "finished Reading OBJ" << endl;
return tris;
}

CanvasTriangle modelToCanvas(ModelTriangle t)
{
  float f=250; /* camera distance, some negative val */
  CanvasTriangle triangle;
  triangle.colour = t.colour;
  for(int i = 0; i < 3; i++)
  {
    vec3 pWorld = t.vertices[i];
    float xCamera = pWorld.x - cameraPos.x;
    float yCamera = pWorld.y - cameraPos.y;
    float zCamera = pWorld.z - cameraPos.z;

    vec3 cameraToVertex = vec3(xCamera, yCamera, zCamera);
    vec3 adjustedVector = cameraToVertex * cameraOrientation;
    // double correctedNear = near + camera.z;

    float pScreen = f/-zCamera;
    int xProj = std::floor(adjustedVector.x*pScreen) + WIDTH/2;
    int yProj = std::floor((-adjustedVector.y)*pScreen) + HEIGHT/2;

    CanvasPoint p = CanvasPoint(xProj, yProj);
    // double z = (near + far)/(far - near) + 1/(-zCamera) * ((-2 * far * near)/(far - near));
    p.depth = 1/-adjustedVector.z;
    triangle.vertices[i] = p;
  }
  return triangle;
}

void drawFilledWireframe(vector <ModelTriangle> tris){
  for (size_t i=0;i<tris.size();i++){
    // cout << i << endl;
    CanvasTriangle new_tri = modelToCanvas(tris[i]);
    drawFilled(new_tri, tris[i].colour);
  }
}

vector<float> interpolation(float from, float to, int numberOfValues)
{
  vector<float> vals;
  for(int i = 0; i < numberOfValues + 1; i++)
  {
    float calc = from + (i * (to - from)/numberOfValues);
    vals.push_back(calc);
  }
  return vals;
}

void computeDepth(CanvasTriangle t, double *depthBuffer)
{
  CanvasPoint p1 = t.vertices[0];
  CanvasPoint p2 = t.vertices[1];
  CanvasPoint p3 = t.vertices[2];

  if(p1.y < p2.y){ std::swap(p1, p2); }
  if(p1.y < p3.y){ std::swap(p1, p3); }
  if(p2.y < p3.y){ std::swap(p2, p3); }

  float ratio = (p1.y - p2.y)/(p1.y - p3.y);
  CanvasPoint extraPoint;
  extraPoint.x = p1.x - ratio*(p1.x - p3.x);
  extraPoint.y = p1.y - ratio*(p1.y - p3.y);
  double depth = p1.depth - ratio*(p1.depth - p3.depth);
  extraPoint.depth = depth;

  // Interpolation
  int numberOfValuesTop = (p1.y - p2.y);
  int numberOfValuesBot = (p2.y - p3.y);

  // Interpolating between the z values
  vector<CanvasPoint> p1_extraPoint = interpolate(p1, extraPoint, ceil(numberOfValuesTop)+1);
  vector<CanvasPoint> p1_p2 = interpolate(p1, p2, ceil(numberOfValuesTop)+1);
  vector<CanvasPoint> p3_extraPoint = interpolate(p3, extraPoint, ceil(numberOfValuesBot)+1);
  vector<CanvasPoint> p3_p2 = interpolate(p3, p2, ceil(numberOfValuesBot)+1);

  for(size_t i = 0; i < p1_extraPoint.size(); i++)
  {
    vector<CanvasPoint> upper = interpolate(p1_extraPoint[i], p1_p2[i], abs(p1_extraPoint[i].x - p1_p2[i].x)+1);
    for(size_t j = 0; j < upper.size(); j++)
    {
      CanvasPoint check = upper[j];
      if((uint32_t) check.x >= 0 && (uint32_t) check.x < WIDTH && (uint32_t) check.y >= 0 && (uint32_t) check.y < HEIGHT)
      {
        if(check.depth > depthBuffer[((uint32_t)check.x + (uint32_t)check.y* WIDTH)])
        {
          depthBuffer[((uint32_t)check.x + (uint32_t)check.y* WIDTH)] = check.depth;
          Colour c = t.colour;
          uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);
          window.setPixelColour((int)check.x, (int)check.y, colour);
        }
      }
    }
  }

  for(size_t i = 0; i < p3_extraPoint.size(); i++)
  {
    vector<CanvasPoint> lower = interpolate(p3_extraPoint[i], p3_p2[i], abs(p3_extraPoint[i].x - p3_p2[i].x)+1);
    for(size_t j = 0; j < lower.size(); j++)
    {
      CanvasPoint check = lower[j];
      if((uint32_t) check.x >= 0 && (uint32_t) check.x < WIDTH && (uint32_t) check.y >= 0 && (uint32_t) check.y < HEIGHT)
      {
        if(check.depth > depthBuffer[((uint32_t)check.x + (uint32_t)check.y* WIDTH)])
        {
          depthBuffer[((uint32_t)check.x + (uint32_t)check.y* WIDTH)] = check.depth;
          Colour c = t.colour;
          uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);
          window.setPixelColour((int)check.x, (int)check.y, colour);
        }
      }
    }
  }
}

void depthBuffer(vector<ModelTriangle> tris)
{ 
  window.clearPixels();
  double *depthBuffer = (double*)malloc(sizeof(double) * WIDTH * HEIGHT);
  for(uint32_t y = 0; y < HEIGHT; y++)
  {
    for(uint32_t x = 0; x < WIDTH; x++)
    {
      depthBuffer[x+y*WIDTH] = 0;
    }
  }

  for(size_t t = 0; t < tris.size(); t++)
  {
    CanvasTriangle projection = modelToCanvas(tris[t]);
    computeDepth(projection, depthBuffer);
  }
}

void rotateX(float theta)
{
  mat3 rot = mat3(vec3(1, 0, 0), 
                  vec3(0, cos(theta), -sin(theta)), 
                  vec3(0, sin(theta), cos(theta)));
  cameraOrientation *= rot;
}

void rotateY(float theta)
{
  mat3 rot = mat3(vec3(cos(theta), 0, sin(theta)), 
                  vec3(0, 1, 0), 
                  vec3(-sin(theta), 0, cos(theta)));
  cameraOrientation *= rot;
}

void rotateZ(float theta)
{
  mat3 rot = mat3(vec3(cos(theta), -sin(theta), 0), 
                  vec3(sin(theta), cos(theta), 0), 
                  vec3(0, 0, 1));
  cameraOrientation *= rot;
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) 
    {
      cout << "TRANSLATE LEFT" << endl;
      cameraPos.x += 10;
    }
    else if(event.key.keysym.sym == SDLK_RIGHT)
    {
      cout << "TRANSLATE RIGHT" << endl;
      cameraPos.x -= 10;
    } 
    else if(event.key.keysym.sym == SDLK_UP)
    {
      cout << "TRANSLATE UP" << endl;
      cameraPos.y -= 10;
    } 
    else if(event.key.keysym.sym == SDLK_DOWN)
    {
      cout << "TRANSLATE DOWN" << endl;
      cameraPos.y += 10;
    }
    else if(event.key.keysym.sym == SDLK_c)
    {
      cout << "CLEAR SCREEN" << endl;
      window.clearPixels();
    }
    else if(event.key.keysym.sym == SDLK_a)
    {
      cout << "ROTATE X" << endl;
      rotateX(5*PI/180);
    } 
    else if(event.key.keysym.sym == SDLK_d)
    {
      cout << "ROTATE X OTHER" << endl;
      rotateX(-5*PI/180);
    } 
    else if(event.key.keysym.sym == SDLK_w)
    {
      cout << "ROTATE Y" << endl;
      rotateY(5*PI/180);
    } 
    else if(event.key.keysym.sym == SDLK_s)
    {
      cout << "ROTATE Y OTHER" << endl;
      rotateY(-5*PI/180);
    } 

  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
