FILE *insert_log_file;

float g_points[][2] = {
    {0.26712908f, 0.0f       },
    {0.17874938f, 0.2254902f },
    {0.0f       , 0.267507f  },
    {0.12611779f, 0.45378152f},
    {0.10228103f, 0.7142857f },
    {0.2701292f , 0.5994398f },
    {0.43673982f, 0.7133057f },
    {0.40913446f, 0.45378152f},
    {0.53414787f, 0.27170867f},
    {0.359685f  , 0.22240896f}
};



#if 0
    // Log
    char log_file_name[64];
    time_t now = time(0);
    strftime(log_file_name, sizeof(log_file_name), "insert_%Y%m%d_%H%M%S.log", localtime(&now));
    insert_log_file = fopen(log_file_name, "wb");
    if (!insert_log_file) {
        fprintf(stderr, "[Error] Could not open insert log.\n");
        return -1;
    } else {
        // Disabling the file cache prevents data from being discarded, even if 
        // the process is terminated in a debugger. ..or does it?
        setvbuf(insert_log_file, 0, _IONBF, 0);
    }
#endif

{ // Push polygon around mouse position for visual cue.
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    double xn =   (xpos / fw)*2.f - 1.f;
    double yn = -((ypos / fh)*2.f - 1.f);

    float mx = (float)xn/scale;
    float my = (float)yn/scale;

    for (int i = 0; i < arrcnt(g_points); i += 1) {
        float x1 = polygon_scale*g_points[i][0];
        float y1 = polygon_scale*g_points[i][1];
        float x2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
        float y2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
        cdt_vec2 p1 = {mx+x1,my+y1};
        cdt_vec2 p2 = {mx+x2,my+y2};
        arrput(edge_end_points, p1);
        arrput(edge_end_points, p2);

        arrput(is_constrained, 0);
    }
}



void callback_scroll(GLFWwindow *window, double xoffset, double yoffset) {
    COMPILER_UNREFERENCED(xoffset);

    int ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
    if (ctrl == GLFW_PRESS) {
        polygon_scale += (float)yoffset;
    } else {
        scale += (float)yoffset * 0.01f;
        scale = scale > 0.0125f ? scale : 0.0125f;
    }
}

void demo_cdt_insert_with_log(float x1, float y1, float x2, float y2) {
    fprintf(insert_log_file, "0x%08X 0x%08X 0x%08X 0x%08X\n", *((uint32_t *)&x1), *((uint32_t *)&y1), *((uint32_t *)&x2), *((uint32_t *)&y2));
    cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
}

void demo_cdt_insert(GLFWwindow *window, float x, float y) {
    COMPILER_UNREFERENCED(window);

    // Log first.
    for (int i = 0; i < arrcnt(g_points); i += 1) {
        float x1 = x + polygon_scale*g_points[i][0];
        float y1 = y + polygon_scale*g_points[i][1];
        float x2 = x + polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
        float y2 = y + polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
        fprintf(insert_log_file, "0x%08X 0x%08X 0x%08X 0x%08X\n", *((uint32_t *)&x1), *((uint32_t *)&y1), *((uint32_t *)&x2), *((uint32_t *)&y2));
    }
    fprintf(insert_log_file, ";\n");

    // Insert afterwards.
    for (int i = 0; i < arrcnt(g_points); i += 1) {
        float x1 = x + polygon_scale*g_points[i][0];
        float y1 = y + polygon_scale*g_points[i][1];
        float x2 = x + polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
        float y2 = y + polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
        cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
    }

    arrput(constraints, next_gen_id);
    next_gen_id += 1;
}

// Callbacks
//
void callback_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    COMPILER_UNREFERENCED(mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int wi, hi;
        glfwGetFramebufferSize(window, &wi, &hi);
        double w = (double)wi;
        double h = (double)hi;

        double xn =   (xpos / w)*2.f - 1.f;
        double yn = -((ypos / h)*2.f - 1.f);

        float x = (float)xn/scale;
        float y = (float)yn/scale;

        demo_cdt_insert(window, x, y);
    }
}

void callback_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    COMPILER_UNREFERENCED(window);
    COMPILER_UNREFERENCED(scancode);
    COMPILER_UNREFERENCED(mods);

    if (key == GLFW_KEY_DELETE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        int id = constraints[0];
        cdt_remove(ctx, id);
        arrdeln(constraints, 0, 1);
    }
}

void callback_drop_down(GLFWwindow *window, int count, const char **paths) {
    glfwFocusWindow(window);

    if (count == 1) {
        // Redo log
        //
        const char *log_file_name = paths[0];
        Parse_Result parse = float_array_from_log_file(log_file_name);  
        int offset = 0;
        for (int j = 0; j < arrlen(parse.counts); j+=1) {
            int n = parse.counts[j];
            for (int i = 0; i < n; i +=4) {
                float x1 = parse.floats[offset + i];
                float y1 = parse.floats[offset + i+1];
                float x2 = parse.floats[offset + i+2];
                float y2 = parse.floats[offset + i+3];
                demo_cdt_insert_with_log(x1, y1, x2, y2);
            }
            fprintf(insert_log_file, ";\n");
            offset += n;

            arrput(constraints, next_gen_id);
            next_gen_id += 1;
        }
    }
}

fclose(insert_log_file);
