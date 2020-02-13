#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

vector<ModelTriangle> readObj();
vector<Colour> readMaterial(string fname);
CanvasTriangle modelToCanvas(ModelTriangle t);
void drawLine(CanvasPoint p1, CanvasPoint p2, Colour c);
void update();
void handleEvent(SDL_Event event);
vector <ModelTriangle> tris;

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
  triangl = readObj();
  colours = readMaterial("/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box/cornell-box/cornell-box.mtl");
  
  cout << colours.size()<< endl;
  for (size_t t = 0;t<tris.size();t++){
    cout << tris[(int)t]<< endl ;
  }
  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event);
    update();

    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
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
  return colours;
}

vector<ModelTriangle> readObj()
{
  ifstream fp;
  vector<Colour> colours = readMaterial("/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box/cornell-box/cornell-box.mtl");
  vector<vec3> vertic;
  fp.open("/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box/cornell-box/cornell-box.obj");
    
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
              float x = stof(splitcomment[1]);
              float y = stof(splitcomment[2]);
              float z = stof(splitcomment[3]);
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
  fp.close();

  ifstream fs;
  fs.open("/home/ak17520/Documents/ComputerGraphics/Lab3/cornell-box/cornell-box/cornell-box.obj");
  if(fs.fail())
    cout << "fails" << endl;

  getline(fs,newline);
  getline(fs,newline);

  while(!fs.eof())
  {
    string light;
    string comment;
    string mat;
    getline(fs,comment);
    if (!comment.empty())
    {
      string *splitcomment = split(comment,' ');
      if (splitcomment[0] == "usemtl")
      {
        mat = splitcomment[1];
        Colour tricolour = getColourFromName(mat,colours);

        while (true)
        {
          getline(fs,comment);
          if (!comment.empty())
          {
            string *splitcomment = split(comment,' ');
            if (splitcomment[0]=="f")
            {
              int first_vert = stoi(splitcomment[1].substr(0, splitcomment[1].size()-1));
              int second_vert = stoi(splitcomment[2].substr(0, splitcomment[2].size()-1));
              int third_vert = stoi(splitcomment[3].substr(0, splitcomment[3].size()-1));

              tris.push_back(ModelTriangle(vertic[first_vert-1],vertic[second_vert-1],vertic[third_vert-1],tricolour));
            }
          }
        }
      }
    }
  }
  fs.close();

return tris;
}

CanvasTriangle modelToCanvas(ModelTriangle t)
{
  float f; /* camera distance, some negative val */

  CanvasPoint a, b, c;
  float x_a = (f * t.vertices[0].x)/(t.vertices[0].z);
  float y_a = (f * t.vertices[0].y)/(t.vertices[0].z);
  a.x = x_a + (WIDTH/2); a.y = y_a + (HEIGHT/2);

  float x_b = (f * t.vertices[1].x)/(t.vertices[1].z);
  float y_b = (f * t.vertices[1].y)/(t.vertices[1].z);
  b.x = x_b + (WIDTH/2); b.y = y_b +(HEIGHT/2);

  float x_c = (f * t.vertices[2].x)/(t.vertices[2].z);
  float y_c = (f * t.vertices[2].y)/(t.vertices[2].z);
  c.x = x_c + (WIDTH/2); c.y = y_c +(HEIGHT/2);

  CanvasTriangle canvasTriangle = CanvasTriangle(a, b, c);
  return canvasTriangle;
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) cout << "LEFT" << endl;
    else if(event.key.keysym.sym == SDLK_RIGHT) cout << "RIGHT" << endl;
    else if(event.key.keysym.sym == SDLK_UP) cout << "UP" << endl;
    else if(event.key.keysym.sym == SDLK_DOWN) cout << "DOWN" << endl;
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}