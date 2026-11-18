#define _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;


#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"




#include <raymath.h>

#include <cassert>
#include <iostream>
#include <vector>
#include "Shader.h"




void APIENTRY DebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

enum ShaderVertexAttribute : int
{
    ATTRIBUTE_POSITIONS,
    ATTRIBUTE_NORMALS,
    ATTRIBUTE_TCOORDS
};


struct DrawCall
{
    GLuint shader;
    GLuint vao; 

};





struct App
{
    GLFWwindow* window;
    int object_index;
    int attribute_color;
    int color_space;
};

struct Mesh
{
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> tcoords;
    int index_count;

    GLuint vao;
    GLuint vbo_positions;
    GLuint vbo_normals;
    GLuint vbo_tcoords;
};

void LoadMeshGpu(Mesh* mesh);
void LoadMeshPar(Mesh* mesh, par_shapes_mesh* par);

void LoadMeshObj(Mesh* mesh, const char* path)
{
    fastObjMesh* obj_mesh;
    obj_mesh = fast_obj_read(path);
    
    mesh->index_count = obj_mesh->index_count;
    mesh->positions.resize(mesh->index_count);
    mesh->normals.resize(mesh->index_count);
    mesh->tcoords.resize(mesh->index_count); 

    for (fastObjUInt i = 0; i < mesh->index_count; i++)
    {
        Vector3* obj_positions_buffer = (Vector3*)obj_mesh->positions;
        fastObjUInt obj_position_index = obj_mesh->indices[i].p;
        mesh->positions[i] = obj_positions_buffer[obj_position_index];

        // TODO -- sample normal indices and add them to the mesh's normals buffer
        // TODO -- sample tcoord indices and add them to the mesh's tcoords buffer
    }

    for (fastObjUInt i = 0; i < mesh->index_count; i++)
    {
        Vector3* obj_normals_buffer = (Vector3*)obj_mesh->normals; 
        fastObjUInt obj_normal_index = obj_mesh->indices[i].n;
        mesh->normals[i] = obj_normals_buffer[obj_normal_index];
    }

    for (fastObjUInt i = 0; i < mesh->index_count; i++)
    {
        Vector2* obj_texture_coordinates_buffer = (Vector2*)obj_mesh->texcoords; 
        fastObjUInt obj_text_coords_index = obj_mesh->indices[i].t; 
        mesh->tcoords[i] = obj_texture_coordinates_buffer[obj_text_coords_index]; 

    }
    


    LoadMeshGpu(mesh);
    fast_obj_destroy(obj_mesh);
}

void LoadMeshHemisphere(Mesh* mesh)
{
    par_shapes_mesh* par = par_shapes_create_hemisphere(32, 32);
    par_shapes_translate(par, -0.5f, -0.5f, 0.0f);

    LoadMeshPar(mesh, par);
    LoadMeshGpu(mesh);

    par_shapes_free_mesh(par);
}

void LoadMeshSphere(Mesh* mesh)
{
    par_shapes_mesh* par = par_shapes_create_parametric_sphere(8, 8);

    LoadMeshPar(mesh, par);
    LoadMeshGpu(mesh);

    par_shapes_free_mesh(par);
}

void LoadMeshPar(Mesh* mesh, par_shapes_mesh* par)
{
    mesh->index_count = par->ntriangles * 3;
    mesh->positions.resize(mesh->index_count);
    mesh->normals.resize(mesh->index_count);
    mesh->tcoords.resize(mesh->index_count);

    Vector3* positions = (Vector3*)par->points;
    Vector3* normals = (Vector3*)par->normals;
    Vector2* tcoords = (Vector2*)par->tcoords;
    for (int i = 0; i < mesh->index_count; i++)
    {
        uint16_t index = par->triangles[i];
        Vector3 position = positions[index];
        Vector3 normal = normals[index];
        Vector2 tcoord = tcoords[index];

        mesh->positions[i] = position;
        mesh->normals[i] = normal;
        mesh->tcoords[i] = tcoord;
    }
}

void LoadMeshGpu(Mesh* mesh)
{
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo_positions);
    glGenBuffers(1, &mesh->vbo_normals);
    glGenBuffers(1, &mesh->vbo_tcoords);

    glBindVertexArray(mesh->vao);

    // Note: positions, normals, and tcoords should be identical in size if loaded correctly!
    // Future improvement: optimize with index buffer (glElementArrayBuffer), but probably not cause that's more work 
    // TODO -- Change to assert after A3 late submission expires

    assert(!mesh->positions.empty());
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_positions);
    glVertexAttribPointer(ATTRIBUTE_POSITIONS, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);

    if (!mesh->normals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_normals);
        glVertexAttribPointer(ATTRIBUTE_NORMALS, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(Vector3), mesh->normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
    }
    else
        printf("Warning: Uploading a mesh without normals to the GPU!\n");
    
    if (!mesh->tcoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_tcoords);
        glVertexAttribPointer(ATTRIBUTE_TCOORDS, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, mesh->tcoords.size() * sizeof(Vector2), mesh->tcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
    }
    else
        printf("Warning: Uploading a mesh without tcoords to the GPU!\n");
    
    glBindVertexArray(GL_NONE);
}

void UnloadMeshGpu(Mesh* mesh)
{
    glDeleteBuffers(1, &mesh->vbo_positions);
    glDeleteBuffers(1, &mesh->vbo_normals);
    glDeleteBuffers(1, &mesh->vbo_tcoords);
    glDeleteVertexArrays(1, &mesh->vao);
}

void Init(App* app);

int main()
{
   





    App app{};
    Init(&app);


    //Triangle code///////////////////////////////////////////////////////////////////////////////////////////////////
    DrawCall draw0{};
    Vector3 triangle_positions[] =
    {
        { 0.0f,  0.5f, 0.0f },
        { -0.5f, -0.5f, 0.0f },
        { 0.5f, -0.5f, 1.0f }
    };

    Vector3 triangle_colors[] =
    {
        { 1.0f, 0.0f, 0.0 },
        { 0.0f, 1.0f, 0.0 },
        { 0.0f, 0.0f, 1.0 }
    };

    GLuint triangle_vao = GL_NONE;
    GLuint triangle_positions_vbo = GL_NONE;
    glGenVertexArrays(1, &triangle_vao);
    glGenBuffers(1, &triangle_positions_vbo);

    glBindVertexArray(triangle_vao);
    GLuint triangle_colors_vbo = GL_NONE;
    glGenBuffers(1, &triangle_colors_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, triangle_positions_vbo);      // The following array (vertex) buffer will be described (triangle_positions_vbo):
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);// Index 0, 3 components, float data, no implicit normalization, no stride, no offset
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(Vector3), triangle_positions, GL_STATIC_DRAW); // Transfer memory of triangle_positions into currently bound vbo
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, triangle_colors_vbo);         // The following array (vertex) buffer will be described (triangle_colors_vbo):
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);// Index 1, 3 components, float data, no implicit normalization, no stride, no offset
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(Vector3), triangle_colors, GL_STATIC_DRAW); // Transfer memory of triangle_colors into currently bound vbo
    glEnableVertexAttribArray(1);

    glBindVertexArray(GL_NONE);                                 // "Stop recording vertex buffer state"

    GLuint vs_shader_triangle = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/a1_triangle.vert");
    GLuint fs_shader_triangle = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/a1_triangle.frag");
    GLuint shader_program_triangle = LoadProgram(vs_shader_triangle, fs_shader_triangle);

    draw0.vao = triangle_vao;
    draw0.shader = shader_program_triangle;

    ///HOOMAN SHADER CODE
    GLuint a1_tri_vert = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/a1_triangle.vert");
    GLuint a1_tri_frag = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/a1_triangle.frag");
    GLuint a1_tri_shader = LoadProgram(a1_tri_vert, a1_tri_frag);

    ///////////////////////////////LINE SHADER //////////////////////////////

    GLuint a2_lines_vert = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/a2_lines.vert");
    GLuint a2_lines_frag = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/a2_lines.frag");
    GLuint a2_lines_shader = LoadProgram(a2_lines_vert, a2_lines_frag);

    // Vertex shaders:
    GLuint vs = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/default.vert");
    GLuint vsLines = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/lines.vert");
    GLuint vsVertexPositionColor = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint vsColorBufferColor = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/buffer_color.vert");

    // Fragment shaders:
    GLuint fsLines = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/lines.frag");
    GLuint fsUniformColor = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsVertexColor = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    GLuint fsTcoords = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/tcoord_color.frag");
    GLuint fsNormals = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/normal_color.frag");
    GLuint fsTexture = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/texture_color.frag");
    GLuint fsTextureMix = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/texturemix_color.frag");

    // Shader programs:
    GLuint shaderUniformColor = LoadProgram(vs, fsUniformColor);
    GLuint shaderVertexPositionColor = LoadProgram(vsVertexPositionColor, fsVertexColor);
    GLuint shaderVertexBufferColor = LoadProgram(vsColorBufferColor, fsVertexColor);
    GLuint shaderLines = LoadProgram(vsLines, fsLines);
    GLuint shaderTcoords = LoadProgram(vs, fsTcoords);
    GLuint shaderNormals = LoadProgram(vs, fsNormals);
    GLuint shaderTexture = LoadProgram(vs, fsTexture);
    GLuint shaderTextureMix = LoadProgram(vs, fsTextureMix);

    // Add the triangle shaders
    GLuint triangleVs = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint triangleFs = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    GLuint shaderTriangle = LoadProgram(triangleVs, triangleFs);

    // Add point shader
    GLuint vsPoints = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/points.vert");
    GLuint shaderPoints = LoadProgram(vsPoints, fsVertexColor);



    //Create Textures and load textures to the gpu

    int carWidth, carHeight, carChannels = 0;
    stbi_uc* pixelsCar = stbi_load("assets/textures/ct4_grey.png", &carWidth, &carHeight, &carChannels,0); 
    GLuint carTexture = GL_NONE; 
    glGenTextures(1, &carTexture); 
    glBindTexture(GL_TEXTURE_2D, carTexture); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, carWidth, carHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelsCar); 
    free(pixelsCar); 
    pixelsCar = nullptr; 






    ///////////////////////






    //Triangle code///////////////////////////////////////////////////////////////////////////////////////////////////



















    


    int width, height;
    glfwGetWindowSize(app.window, &width, &height);

    Mesh meshes[3];
    LoadMeshHemisphere(&meshes[0]);
    LoadMeshSphere(&meshes[2]);
    LoadMeshObj(&meshes[1], "./assets/meshes/shoes.obj");

    GLuint vs_shader = LoadShader(GL_VERTEX_SHADER, "./assets/shaders/a3_mesh.vert");
    GLuint fs_shader = LoadShader(GL_FRAGMENT_SHADER, "./assets/shaders/a3_mesh.frag");
    GLuint shader_program = LoadProgram(vs_shader, fs_shader);
    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(app.window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (app.object_index == 0)
        {
            static float animate_z = 0.0f;

            Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) *
                MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp = world * view * proj;

            glUseProgram(shader_program);
            GLint loc_world = glGetUniformLocation(shader_program, "u_world");
            GLint loc_mvp = glGetUniformLocation(shader_program, "u_mvp");
            GLint loc_style = glGetUniformLocation(shader_program, "u_style");
            GLint loc_color_space = glGetUniformLocation(shader_program, "u_color_space");

            glUniformMatrix4fv(loc_world, 1, GL_FALSE, MatrixToFloat(world));
            glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, MatrixToFloat(mvp));
            glUniform1i(loc_style, app.attribute_color);
            glUniform1i(loc_color_space, app.color_space);

            const Mesh& mesh = meshes[0];
            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, mesh.index_count);
            app.attribute_color = 1;
        }


        if (app.object_index == 1)
        {
            static float animate_z = 0.0f;

            Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) *
                MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp = world * view * proj;

            glUseProgram(shader_program);
            GLint loc_world = glGetUniformLocation(shader_program, "u_world");
            GLint loc_mvp = glGetUniformLocation(shader_program, "u_mvp");
            GLint loc_style = glGetUniformLocation(shader_program, "u_style");
            GLint loc_color_space = glGetUniformLocation(shader_program, "u_color_space");

            glUniformMatrix4fv(loc_world, 1, GL_FALSE, MatrixToFloat(world));
            glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, MatrixToFloat(mvp));
            glUniform1i(loc_style, app.attribute_color);
            glUniform1i(loc_color_space, app.color_space);

            const Mesh& mesh = meshes[1];
            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, mesh.index_count);
            app.attribute_color = 2;

        }

        if (app.object_index == 2)
        {
            
            glUseProgram(shaderTexture);
            static float animate_z = 0.0f;
            GLint u_mvp = glGetUniformLocation(shaderTexture, "u_mvp"); 
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix World2 = MatrixIdentity(); 
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp2 = view * World2 * proj; 
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, MatrixToFloat(mvp2)); 
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, carTexture);
            
            cout << "object index is 2"; 


       





           
            




            GLint u_texture = glGetUniformLocation(shaderTexture, "u_texture");
            glUniform1i(u_texture, 0);
            const Mesh& mesh = meshes[1];

            glBindVertexArray(mesh.vao);
            
            

            
           

        }


        





        if (app.object_index == 3)
        {
            static float animate_z = 0.0f;


            Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) *
                MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp = world * view * proj;
            
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            glUseProgram(draw0.shader);
            glBindVertexArray(draw0.vao);
            Matrix m = MatrixTranslate(0.0f, 0.0f, 0.0f);
            GLuint loc = glad_glGetUniformLocation(draw0.shader, "transform");
            GLuint uStyle = glGetUniformLocation(shader_program_triangle, "u_style_choice");
            glUniform1i(uStyle, 0);
            glUniformMatrix4fv(loc, 1, GL_FALSE, MatrixToFloat(mvp));
            glDrawArrays(GL_TRIANGLES, 0, 3);

        }


        if (app.object_index == 4)
        {
            static float animate_z = 0.0f;


            Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) *
                MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp = world * view * proj;

            glUseProgram(shader_program);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(draw0.shader);
            glBindVertexArray(draw0.vao);
            Matrix m = MatrixTranslate(0.0f, 0.0f, 0.0f);
            GLuint loc = glad_glGetUniformLocation(draw0.shader, "transform");
            GLuint uStyle = glGetUniformLocation(shader_program_triangle, "u_style_choice");
            glUniform1i(uStyle, 1);
            glUniformMatrix4fv(loc, 1, GL_FALSE, MatrixToFloat(mvp));
            glDrawArrays(GL_TRIANGLES, 0, 3);

        }



       




        /*else if (app.object_index < 3) {
            static float animate_z = 0.0f;

            Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) *
                MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
            Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
            Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);
            Matrix mvp = world * view * proj;

            glUseProgram(shader_program);

            
            GLint loc_world = glGetUniformLocation(shader_program, "u_world");
            GLint loc_mvp = glGetUniformLocation(shader_program, "u_mvp");
            GLint loc_style = glGetUniformLocation(shader_program, "u_style");
            GLint loc_color_space = glGetUniformLocation(shader_program, "u_color_space");

            glUniformMatrix4fv(loc_world, 1, GL_FALSE, MatrixToFloat(world));
            glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, MatrixToFloat(mvp));
            glUniform1i(loc_style, app.attribute_color);
            glUniform1i(loc_color_space, app.color_space);

            const Mesh& mesh = meshes[app.object_index];
            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, mesh.index_count);
            

        }*/






        /*glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        static float animate_z = 0.0f;

        Matrix world = MatrixRotateY(glfwGetTime() * 100.0f * DEG2RAD) * MatrixTranslate(0.0f, 0.0f, 5.0f + (sinf(glfwGetTime()) * 3.0f) * animate_z);
        Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
        //Matrix proj = MatrixOrtho(-10.0, 10.0, -10.0, 10.0, 0.1, 10.0);                   // 2D projection (objects appear the same size regardless of distance from the camera)
        Matrix proj = MatrixPerspective(PI * 0.5f, (width / (float)height), 0.1, 100.0);    // 3D projection (objects closer to the camera appear bigger)
        Matrix mvp = world * view * proj;

        glUseProgram(shader_program);
        GLint loc_world = glGetUniformLocation(shader_program, "u_world");
        GLint loc_mvp = glGetUniformLocation(shader_program, "u_mvp");
        GLint loc_style = glGetUniformLocation(shader_program, "u_style");
        GLint loc_color_space = glGetUniformLocation(shader_program, "u_color_space");

        glUniformMatrix4fv(loc_world, 1, GL_FALSE, MatrixToFloat(world));
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, MatrixToFloat(mvp));
        glUniform1i(loc_style, app.attribute_color);
        glUniform1i(loc_color_space, app.color_space);

        const Mesh& mesh = meshes[app.object_index];
        glBindVertexArray(mesh.vao);

        glDrawArrays(GL_TRIANGLES, 0, mesh.index_count);*/

        glfwSwapBuffers(app.window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = (App*)glfwGetWindowUserPointer(window);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        ++app->object_index %= 5;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        ++app->attribute_color %= 3;
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        ++app->color_space %= 2;
    }
}

void Init(App* app)
{
    assert(glfwInit() == GLFW_TRUE);

    // Request certain OpenGL features
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef NDEBUG
    // Don't make a debug context if in release mode
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    app->window = glfwCreateWindow(1600, 900, "Graphics 1", nullptr, nullptr);
    assert(app->window != nullptr);

    glfwSetWindowUserPointer(app->window, app);
    glfwSetKeyCallback(app->window, KeyCallback);

    glfwMakeContextCurrent(app->window);
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == true);

#ifdef NDEBUG
#else
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
#endif
}

void APIENTRY DebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}
