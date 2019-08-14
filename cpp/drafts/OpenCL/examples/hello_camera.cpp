#include <drafts/OpenCL/GL.hpp>

#include <DO/Sara/Core/DebugUtilities.hpp>
#include <DO/Sara/Core/HDF5.hpp>
#include <DO/Sara/Core/StringFormat.hpp>
#include <DO/Sara/Core/Timer.hpp>
#include <DO/Sara/ImageIO.hpp>
#include <DO/Sara/ImageProcessing/Flip.hpp>

#include <DO/Kalpana/Math/Projection.hpp>
#include <DO/Kalpana/3D/TrackBall.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GLFW/glfw3.h>

#include <Eigen/Geometry>

#include <map>


using namespace DO::Sara;
using namespace std;

namespace kalpana = DO::Kalpana;


auto resize_framebuffer(GLFWwindow*, int width, int height)
{
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

inline auto init_glfw_boilerplate()
{
  // Initialize the windows manager.
  if (!glfwInit())
    throw std::runtime_error{"Error: failed to initialize GLFW!"};

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

inline auto init_glew_boilerplate()
{
#ifndef __APPLE__
  // Initialize GLEW.
  auto err = glewInit();
  if (err != GLEW_OK)
  {
    std::cerr << format("Error: failed to initialize GLEW: %s",
                        glewGetErrorString(err))
              << std::endl;
  }
#endif
}


struct View
{
  View()
  {
    view.setIdentity();
    view.translate(Vector3f{0.f, 0.f, -100.f});
  }

  auto move_left(float delta)
  {
    view.translate(-scale * delta * Vector3f::UnitX());
  }

  auto move_right(float delta)
  {
    view.translate(scale * delta * Vector3f::UnitX());
  }

  auto move_backward(float delta)
  {
    view.translate(-scale * delta * Vector3f::UnitZ());
  }

  auto move_forward(float delta)
  {
    view.translate(scale * delta * Vector3f::UnitZ());
  }

  auto matrix() const -> const Matrix4f&
  {
    return view.matrix();
  }

  Transform<float, 3, Eigen::Projective> view;

  float scale = 1e-1f;
};

struct Time
{
  void update()
  {
    last_frame = current_frame;
    current_frame = timer.elapsed_ms();
    delta_time = current_frame - last_frame;
  }

  Timer timer;
  float delta_time = 0.f;
  float last_frame = 0.f;
  float current_frame = 0.f;
};

auto move_camera_from_keyboard(GLFWwindow* window, View& view, Time& time)
{
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    view.move_forward(time.delta_time);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    view.move_backward(time.delta_time);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    view.move_left(time.delta_time);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    view.move_right(time.delta_time);
}


auto mouse_pressed = false;
auto trackball = kalpana::TrackBall{};
float last_x = 800.f / 2.f;
float last_y = 600.f / 2.f;
QQuaternion rotation;

auto move_camera_from_mouse(GLFWwindow* window, double x_pos, double y_pos)
{
  const auto state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

  if (state == GLFW_RELEASE && mouse_pressed)
  {
    mouse_pressed = false;
    std::cout << "Mouse released at " << x_pos << " " << y_pos << std::endl;
    trackball.release(QPointF(last_x, last_y));
    return;
  }

  if (state != GLFW_PRESS)
    return;

  if (!mouse_pressed)
  {
    last_x = x_pos;
    last_y = y_pos;
    mouse_pressed = true;
    trackball.push(QPointF(last_x, last_y), trackball.rotation());
  }

  std::cout << "Before pressed at " << last_x << " " << last_y << std::endl;
  std::cout << "Now    pressed at " << x_pos << " " << y_pos << std::endl;
  trackball.move(QPointF(last_x, last_y));
  qDebug() << trackball.rotation();
}


auto read_point_cloud(const std::string& h5_filepath) -> Tensor_<float, 2>
{
  auto h5_file = H5File{h5_filepath, H5F_ACC_RDONLY};

  auto coords = Tensor_<double, 2>{};
  h5_file.read_dataset("points", coords);
  coords.matrix() *= -1;

  auto colors = Tensor_<double, 2>{};
  h5_file.read_dataset("colors", colors);
  SARA_DEBUG << "OK" << std::endl;

  // Concatenate the data.
  auto vertex_data = Tensor_<double, 2>{{coords.size(0), 6}};
  vertex_data.matrix() << coords.matrix(), colors.matrix();

  return vertex_data.cast<float>();
}

auto make_point_cloud()
{
  // Encode the vertex data in a tensor.
#ifdef __APPLE__
  const auto vertex_data = read_point_cloud("/Users/david/Desktop/geometry.h5");
#else
  const auto vertex_data = read_point_cloud("/home/david/Desktop/geometry.h5");
#endif
  SARA_DEBUG << "vertices =\n" << vertex_data.matrix() << std::endl;
  return vertex_data;
}


int main()
{
  // ==========================================================================
  // Display initialization.
  //
  init_glfw_boilerplate();

  // Create a window.
  const auto width = 800;
  const auto height = 600;
  auto window =
      glfwCreateWindow(width, height, "Hello Point Cloud", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, resize_framebuffer);
  glfwSetCursorPosCallback(window, move_camera_from_mouse);

  init_glew_boilerplate();


  // ==========================================================================
  // Shader program setup.
  //
  std::map<std::string, int> arg_pos = {{"in_coords", 0},  //
                                        {"in_color", 1}};


  const auto vertex_shader_source = R"shader(
#version 330 core
  layout (location = 0) in vec3 in_coords;
  layout (location = 1) in vec3 in_color;

  uniform mat4 transform;
  uniform mat4 view;
  uniform mat4 projection;

  out vec3 out_color;

  void main()
  {
    gl_Position = projection * view * transform * vec4(in_coords, 1.0);
    gl_PointSize = 5.0;
    out_color = in_color;
  }
  )shader";
  auto vertex_shader = GL::Shader{};
  vertex_shader.create_from_source(GL_VERTEX_SHADER, vertex_shader_source);


  const auto fragment_shader_source = R"shader(
#version 330 core
  in vec3 out_color;
  out vec4 frag_color;

  void main()
  {
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    if (dot(circCoord, circCoord) > 1.0)
        discard;

    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = 1.0 - smoothstep(0.2, 0.5, dist);

    frag_color = vec4(out_color, alpha);
  }
  )shader";
  auto fragment_shader = GL::Shader{};
  fragment_shader.create_from_source(GL_FRAGMENT_SHADER,
                                     fragment_shader_source);

  auto shader_program = GL::ShaderProgram{};
  shader_program.create();
  shader_program.attach(vertex_shader, fragment_shader);

  vertex_shader.destroy();
  fragment_shader.destroy();


  // ==========================================================================
  // Encode the vertex data in a tensor.
  //
  auto vertices = make_point_cloud();

  const auto row_bytes = [](const TensorView_<float, 2>& data) {
    return data.size(1) * sizeof(float);
  };
  const auto float_pointer = [](int offset) {
    return reinterpret_cast<void*>(offset * sizeof(float));
  };

  auto vao = GL::VertexArray{};
  vao.generate();


  // ==========================================================================
  // Setup Vertex attributes on the GPU side.
  //
  auto vbo = GL::Buffer{};
  vbo.generate();
  {
    glBindVertexArray(vao);

    // Copy vertex data.
    vbo.bind_vertex_data(vertices);

    // Map the parameters to the argument position for the vertex shader.
    //
    // Vertex coordinates.
    glVertexAttribPointer(arg_pos["in_coords"], 3 /* 3D points */, GL_FLOAT,
                          GL_FALSE, row_bytes(vertices), float_pointer(0));
    glEnableVertexAttribArray(arg_pos["in_coords"]);

    // Texture coordinates.
    glVertexAttribPointer(arg_pos["in_color"], 3 /* RGB_COLOR */, GL_FLOAT,
                          GL_FALSE, row_bytes(vertices), float_pointer(3));
    glEnableVertexAttribArray(arg_pos["in_color"]);
  }

  shader_program.use(true);


  // ==========================================================================
  // Setup options for point cloud rendering.
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_PROGRAM_POINT_SIZE);

  // You absolutely need this for 3D objects!
  glEnable(GL_DEPTH_TEST);


  auto view = View{};
  auto time = Time{};


  // Display image.
  glfwSwapInterval(1);
  while (!glfwWindowShouldClose(window))
  {
    // Calculate the elapsed time.
    time.update();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Camera interaction with keyboard.
    move_camera_from_keyboard(window, view, time);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // Important.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transform matrix.
    auto transform = Transform<float, 3, Eigen::Projective>{};
    transform.setIdentity();
    //transform.rotate(AngleAxisf(std::pow(1.5, 5) * time.last_frame / 10000,
    //                            Vector3f{0.5f, 1.0f, 0.0f}.normalized()));
    shader_program.set_uniform_matrix4f("transform", transform.matrix().data());

    // View matrix.
    shader_program.set_uniform_matrix4f("view", view.matrix().data());

    // Projection matrix.
    const Matrix4f projection =
        kalpana::perspective(45., 800. / 600., .1, 1000.).cast<float>();
    shader_program.set_uniform_matrix4f("projection", projection.data());

    // Draw triangles.
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, vertices.size(0));

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  vao.destroy();
  vbo.destroy();

  // Clean up resources.
  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
