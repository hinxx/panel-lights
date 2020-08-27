// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

#include "sequence.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL(glfwGetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    char filePathStr[256] = "./sdcard";
    FileList fileList;
    FileName fileName;
    Sequence sequence;
    bool playing = false;
    float elapsedTime = 0;
    bool show_generator_window = true;
    SequenceList sequences;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // our sequence handling window
        {
            ImGui::Begin("Panel Lights Sequence manipulator");

            // file path input field
            // XXX: replace with file picker at some point in time!
            // XXX: make sure it works on all OSes..
            ImGui::Text("File Path");
            ImGui::SameLine();
            ImGui::InputText("", filePathStr, 256);
            // go over directory contents and return list of file names
            if (ImGui::Button("Load Files")) {
                fileList = loadFileList(filePathStr);
                fprintf(stderr, "file list size %d\n", fileList.count());
                sequence = Sequence();
            }

            for (int n = 0; n < fileList.count(); n++) {
                if (ImGui::Selectable(fileList.data[n].name, fileList.selected() == n)) {
                    fileList.select(n);
                    fileName = fileList.selectedFileName();
                    fprintf(stderr, "selected item is: %d %s\n", fileList.selected(), fileName.name);

                    // load the file contents
                    sequence = loadSequence(fileName);
                    for (int n = 0; n < sequence.count(); n++) {
                        Step step = sequence.data[n];
                        fprintf(stderr, "panel1: mode %d, color %06X | panel2: mode %d, color %06X | wait %f s\n",
                                step.mode1,
                                (unsigned int)ImColor(step.color1[0], step.color1[1], step.color1[2], .0f),
                                step.mode2,
                                (unsigned int)ImColor(step.color2[0], step.color2[1], step.color2[2], .0f),
                                step.duration);
                    }
                    // reset playing
                    playing = false;
                    elapsedTime = 0;
                }
            }

            if (sequence) {
                // sequence not valid

                // fprintf(stderr, "sequence size %d\n", sequence.count());
                // create step widgets
                ImGui::Text("List of steps");
                ImGui::Columns(5, "listofsteps");
                ImGui::SetColumnWidth(0, 60);
                ImGui::SetColumnWidth(1, 60);
                ImGui::SetColumnWidth(2, 60);
                ImGui::SetColumnWidth(3, 300);
//                ImGui::SetColumnWidth(4, 100);
                ImGui::Separator();
                ImGui::Text("step #"); ImGui::NextColumn();
                ImGui::Text("Color 1"); ImGui::NextColumn();
                ImGui::Text("Color 2"); ImGui::NextColumn();
                ImGui::Text("Wait"); ImGui::NextColumn();
                ImGui::Text("Action"); ImGui::NextColumn();
                ImGui::Separator();
                for (int n = 0; n < sequence.count(); n++) {
                    ImGuiColorEditFlags flags1 = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
                    ImGuiColorEditFlags flags2 = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
                    Step *step = sequence.step(n);
                    ImGui::PushID(n);
                    ImGui::Text("%04d", n + 1);
                    ImGui::NextColumn();
                    // in case of random color mode disable the color picker and grey out the button
                    if (step->random1) {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
                        flags1 |= ImGuiColorEditFlags_NoPicker;
                    }
                    ImGui::ColorEdit3("color1", (float *)&step->color1, flags1);
                    if (step->random1) {
                        ImGui::PopStyleVar();
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("###random 1", &step->random1);
                    ImGui::NextColumn();
                    // in case of random color mode disable the color picker and grey out the button
                    if (step->random2) {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
                        flags2 |= ImGuiColorEditFlags_NoPicker;
                    }
                    ImGui::ColorEdit3("color2", (float*)&step->color2, flags2);
                    if (step->random2) {
                        ImGui::PopStyleVar();
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("##random 2", &step->random2);
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("", &step->duration, 1.0f, 9.9f, "%.1f s")) {
                        // value has changed, recalculate sequence duration
                        sequence.calcDuration();
                    }
                    ImGui::NextColumn();
                    if (ImGui::Button("Remove step")) {
                        fprintf(stderr, "sequence: Remove step\n");
                        sequence.delStep(n);
                        // sequence has a new step, recalculate sequence duration
                        sequence.calcDuration();
                    }
                    ImGui::NextColumn();
                    ImGui::PopID();
                }
                ImGui::Separator();
                // end multicolumn
                ImGui::Columns(1);

                // insert an empty row
                ImGui::Dummy(ImVec2(10,10));

                // start multicolumn again
                ImGui::Columns(5, "listofsteps2");
                ImGui::SetColumnWidth(0, 60);
                ImGui::SetColumnWidth(1, 60);
                ImGui::SetColumnWidth(2, 60);
                ImGui::SetColumnWidth(3, 300);
                ImGui::Separator();

                // controls for adding a new step (append to sequence)
                static ImGuiColorEditFlags flags1 = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
                static ImGuiColorEditFlags flags2 = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
                static Step newStep;
                static int newId = 9999;
                ImGui::Text("%04d", newId);
                //ImGui::InputInt("###new step id", &newId);
                ImGui::NextColumn();
                // in case of random color mode disable the color picker and grey out the button
                if (newStep.random1) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
                    flags1 |= ImGuiColorEditFlags_NoPicker;
                }
                ImGui::ColorEdit3("new color1", (float *)&newStep.color1, flags1);
                if (newStep.random1) {
                    ImGui::PopStyleVar();
                }
                ImGui::SameLine();
                ImGui::Checkbox("###new random 1", &newStep.random1);
                ImGui::NextColumn();
                // in case of random color mode disable the color picker and grey out the button
                if (newStep.random2) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
                    flags2 |= ImGuiColorEditFlags_NoPicker;
                }
                ImGui::ColorEdit3("new color2", (float*)&newStep.color2, flags2);
                if (newStep.random2) {
                    ImGui::PopStyleVar();
                }
                ImGui::SameLine();
                ImGui::Checkbox("##new random 2", &newStep.random2);
                ImGui::NextColumn();
                ImGui::SliderFloat("###new duration", &newStep.duration, 1.0f, 9.9f, "%.1f s");
                ImGui::NextColumn();
                if (ImGui::Button("Add step")) {
                    fprintf(stderr, "sequence: Add step\n");
                    sequence.addStep(newStep);
                    // sequence has a new step, recalculate sequence duration
                    sequence.calcDuration();
                }
                ImGui::NextColumn();
                // end multicolumn
                ImGui::Columns(1);
                ImGui::Separator();

                // sequence play/stop controls
                if (ImGui::Button("Play")) {
                    fprintf(stderr, "sequence play: Start\n");
                    playing = true;
                    elapsedTime = 0;
                }
                ImGui::SameLine(0, 20);
                if (ImGui::Button("Stop")) {
                    fprintf(stderr, "sequence play: Stop\n");
                    playing = false;
                    elapsedTime = 0;
                }

                if (playing) {
                    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop;
                    elapsedTime += ImGuiIO().DeltaTime;
                    double runTime = 0;
                    Step *step = NULL;
                    int stepIndex = 0;
                    for (stepIndex = 0; stepIndex < sequence.count(); stepIndex++) {
                        step = sequence.step(stepIndex);
                        runTime += step->duration;
                        if (elapsedTime < runTime) {
                            break;
                        }
                    }
                    ImGui::Text("Step # %d, elapsed time %.2f / %.2f\n", stepIndex+1, elapsedTime, sequence.duration);
                    ImVec4 color1 = ImColor(step->color1[0], step->color1[1], step->color1[2]);
                    ImGui::ColorButton("###Panel 1", color1, flags, ImVec2(100, 100));
                    ImGui::SameLine();
                    ImVec4 color2 = ImColor(step->color2[0], step->color2[1], step->color2[2]);
                    ImGui::ColorButton("###Panel 2", color2, flags, ImVec2(100, 100));
                    if (elapsedTime > sequence.duration) {
                        elapsedTime = 0;
                        fprintf(stderr, "sequence play: Loop\n");
                    }
                }
            } else {
                // sequence not valid
                playing = false;
                elapsedTime = 0;
            }

            ImGui::End();
        } // our sequence handling window

        // pattern generator window
        if (show_generator_window)
        {
            ImGui::Begin("Pattern Generator Window", &show_generator_window);
            static int gen_num_steps = 1;
            static float gen_step_duration= 1.0f;
            static float gen_wait_duration = 1.0f;
            static bool gen_wait_steps = true;
            static char gen_sequence_name[10] = {0};
            ImGui::Text("Pattern generator number settings");
            ImGui::InputText("Name of sequence", gen_sequence_name, 10);
            ImGui::InputInt("Number of steps", &gen_num_steps);
//            ImGui::InputFloat("Step duration", &gen_step_duration);
            ImGui::SliderFloat("Step duration", &gen_step_duration, 1.0f, 9.9f, "%.1f s");
            ImGui::Checkbox("Add wait steps", &gen_wait_steps);
            if (gen_wait_steps) {
//                ImGui::InputFloat("Wait duration", &gen_wait_duration);
                ImGui::SliderFloat("Wait duration", &gen_wait_duration, 1.0f, 9.9f, "%.1f s");
            }

            // Generate a default palette. The palette will persist and can be edited.
            static bool saved_palette_init = true;
            static ImVec4 saved_palette[32] = {};
            static ImVec4 step_palette[8] = {};
            if (saved_palette_init)
            {
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                        saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
                    saved_palette[n].w = 1.0f; // Alpha
                }
                for (int n = 0; n < 8; n++)
                {
                    ImGui::ColorConvertHSVtoRGB(.0f, .0f, .0f,
                        step_palette[n].x, step_palette[n].y, step_palette[n].z);
                    step_palette[n].w = 1.0f; // Alpha
                }
                saved_palette_init = false;
            }

            static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);

            ImGui::BeginGroup(); // Lock X position
            ImGui::Separator();
            ImGui::Text("Palette");
//            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            for (int n = 0; n < 8; n++)
            {
                ImGui::PushID(n);
                if ((n % 8) != 0)
                    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
//                if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
//                    color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

                ImGui::ColorButton("##newpalette", step_palette[n], palette_button_flags, ImVec2(20, 20));

                // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                        memcpy((float*)&step_palette[n], payload->Data, sizeof(float) * 3);
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                        memcpy((float*)&step_palette[n], payload->Data, sizeof(float) * 4);
                    ImGui::EndDragDropTarget();
                }

                ImGui::PopID();
            }
            ImGui::EndGroup();

            ImGui::SameLine(0.0f, 30.0f/*ImGui::GetStyle().ItemSpacing.y*/);

            ImGui::BeginGroup(); // Lock X position
            ImGui::Separator();
            ImGui::Text("Palette");
            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            {
                ImGui::PushID(n);
                if ((n % 8) != 0)
                    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                    color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

                // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
//                if (ImGui::BeginDragDropTarget())
//                {
//                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
//                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
//                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
//                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
//                    ImGui::EndDragDropTarget();
//                }

                ImGui::PopID();
            }
            ImGui::EndGroup();

            if (ImGui::Button("Generate")) {
                fprintf(stderr, "generating pattern: %d steps\n", gen_num_steps);
                Sequence newSequence(gen_sequence_name);
                int step_index = 0;
                for (int n = 0; n < gen_num_steps; n++) {
//                    Step newStep = Step(0, 0x00FF0000, 0, 0x0000FFFF, 23);
                    // take a user defined color
                    step_index = n % 8;
                    unsigned int c1 = ImColor(step_palette[step_index]);
                    unsigned char d1 = (unsigned char)(gen_step_duration * 10);
                    Step newStep = Step(0, c1, 0, c1, d1);
                    newSequence.addStep(newStep);
                    if (gen_wait_steps) {
                        unsigned char d2 = (unsigned char)(gen_wait_duration * 10);
                        Step newStep = Step(0, 0x00000000, 0, 0x00000000, d2);
                        newSequence.addStep(newStep);
                    }
                }
                newSequence.calcDuration();
                sequences.addSequence(newSequence);
            }

            ImGui::End();
        }

        // sequence window
        {
            ImGui::Begin("Sequence Window");
            ImGui::Text("Number of sequences: %d", sequences.count());
            ImGui::Columns(4, "sequences");
            ImGui::Separator();
            ImGui::Text("ID"); ImGui::NextColumn();
            ImGui::Text("Name"); ImGui::NextColumn();
            ImGui::Text("Steps"); ImGui::NextColumn();
            ImGui::Text("Duration"); ImGui::NextColumn();
            ImGui::Separator();
            for (int n = 0; n < sequences.count(); n++) {
                ImGui::PushID(n);
                ImGui::Text("%04d", n + 1);
                ImGui::NextColumn();
                Sequence *seq = sequences.sequence(n);
                if (ImGui::Selectable(seq->getName(), sequences.selectedIndex() == n, ImGuiSelectableFlags_SpanAllColumns)) {
                    sequences.selectSequence(n);
                    fprintf(stderr, "Selected sequence %s, number of steps %d\n", seq->getName(), seq->numSteps());
                    for (int m = 0; m < seq->numSteps(); m++) {
                        Step *step = seq->getStep(m);
                        fprintf(stderr, "panel1: mode %d, color %06X | panel2: mode %d, color %06X | wait %f s\n",
                                step->mode1,
                                (unsigned int)ImColor(step->color1[0], step->color1[1], step->color1[2], .0f),
                                step->mode2,
                                (unsigned int)ImColor(step->color2[0], step->color2[1], step->color2[2], .0f),
                                step->duration);
                    }
                }
                ImGui::NextColumn();
                ImGui::Text("%d", seq->numSteps());
                ImGui::NextColumn();
                ImGui::Text("%.2f s", seq->duration);
                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);
            ImGui::Separator();

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
