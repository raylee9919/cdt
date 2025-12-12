
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
