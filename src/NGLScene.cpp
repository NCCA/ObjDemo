#include <QMouseEvent>
#include <QGuiApplication>
#include <QFont>
#include <QFileDialog>

#include "NGLScene.h"
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <iostream>

NGLScene::NGLScene(const std::string &_oname, const std::string &_tname)
{
  setTitle("Obj Demo");
  m_showBBox = true;
  m_showBSphere = true;
  m_objFileName = _oname;
  m_textureFileName = _tname;
}

NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(int _w, int _h)
{
  m_project = ngl::perspective(45.0f, static_cast<float>(_w) / _h, 0.05f, 350.0f);
  m_win.width = static_cast<int>(_w * devicePixelRatio());
  m_win.height = static_cast<int>(_h * devicePixelRatio());
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);

  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0, 4, 8);
  ngl::Vec3 to(0, 0, 0);
  ngl::Vec3 up(0, 1, 0);
  m_view = ngl::lookAt(from, to, up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project = ngl::perspective(45.0f, 720.0f / 576.0f, 0.05f, 350.0f);

  ngl::ShaderLib::createShaderProgram("TextureShader");

  ngl::ShaderLib::attachShader("TextureVertex", ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("TextureFragment", ngl::ShaderType::FRAGMENT);
  ngl::ShaderLib::loadShaderSource("TextureVertex", "shaders/TextureVertex.glsl");
  ngl::ShaderLib::loadShaderSource("TextureFragment", "shaders/TextureFragment.glsl");

  ngl::ShaderLib::compileShader("TextureVertex");
  ngl::ShaderLib::compileShader("TextureFragment");
  ngl::ShaderLib::attachShaderToProgram("TextureShader", "TextureVertex");
  ngl::ShaderLib::attachShaderToProgram("TextureShader", "TextureFragment");

  // link the shader no attributes are bound
  ngl::ShaderLib::linkProgramObject("TextureShader");
  ngl::ShaderLib::use("TextureShader");

  ngl::ShaderLib::use(ngl::nglColourShader);
  ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 1.0f, 1.0f);

  // first we create a mesh from an obj passing in the obj file and texture
  m_mesh.reset(new ngl::Obj(m_objFileName, m_textureFileName));
  // now we need to create this as a VAO so we can draw it
  m_mesh->createVAO();
  m_mesh->calcBoundingSphere();
  ngl::VAOPrimitives::createSphere("sphere", 1.0, 20);
  // as re-size is not explicitly called we need to do this.
  glViewport(0, 0, width(), height());
  m_text.reset(new ngl::Text("fonts/Arial.ttf", 16));
  m_text->setScreenSize(width(), height());
  m_text->setColour(1, 1, 1);
}

void NGLScene::loadMatricesToShader()
{

  ngl::Mat4 MVP = m_project * m_view *
                  m_mouseGlobalTX *
                  m_transform.getMatrix();

  ngl::ShaderLib::setUniform("MVP", MVP);
}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, m_win.width, m_win.height);
  auto rotX = ngl::Mat4::rotateX(m_win.spinXFace);
  auto rotY = ngl::Mat4::rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX = rotY * rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

  ngl::ShaderLib::use("TextureShader");
  m_transform.reset();
  loadMatricesToShader();
  // draw the mesh
  m_mesh->draw();
  // m_bin->draw();
  //  draw the mesh bounding box
  ngl::ShaderLib::use("nglColourShader");

  if (m_showBBox == true)
  {
    loadMatricesToShader();
    ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 1.0f, 1.0f);
    m_mesh->drawBBox();
  }
  if (m_showBSphere == true)
  {
    ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 1.0f, 1.0f);
    m_transform.reset();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_transform.setPosition(m_mesh->getSphereCenter());
    m_transform.setScale(m_mesh->getSphereRadius(), m_mesh->getSphereRadius(), m_mesh->getSphereRadius());
    loadMatricesToShader();
    ngl::VAOPrimitives::draw("sphere");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  m_text->renderText(10, 18, "P toggle bounding Sphere B Toggle Bounding Box");
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape:
    QGuiApplication::exit(EXIT_SUCCESS);
    break;
  // turn on wirframe rendering
  case Qt::Key_W:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  // turn off wire frame
  case Qt::Key_S:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  // show full screen
  case Qt::Key_F:
    showFullScreen();
    break;
  // show windowed
  case Qt::Key_N:
    showNormal();
    break;
  case Qt::Key_B:
    m_showBBox ^= true;
    break;
  case Qt::Key_P:
    m_showBSphere ^= true;
    break;
  case Qt::Key_Space:
    m_win.spinXFace = 0;
    m_win.spinYFace = 0;
    m_modelPos.set(ngl::Vec3::zero());
    break;

  case Qt::Key_O:
  {
    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        tr("load Obj"),
        QDir::currentPath(),
        tr("*.obj"));

    if (!filename.isNull())
    {
      m_mesh.reset(new ngl::Obj(filename.toStdString()));
      // now we need to create this as a VAO so we can draw it
      m_mesh->createVAO();
      m_mesh->calcBoundingSphere();
    }
  }
  break;

  case Qt::Key_T:
  {
    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        tr("load texture"),
        QDir::currentPath(),
        tr("Image Files (*.png *.jpg *.bmp *.tif)"));

    if (!filename.isNull())
    {
      m_mesh->setTexture(filename.toStdString());
    }
  }
  break;

  default:
    break;
  }
  // finally update the GLWindow and re-draw
  update();
}
