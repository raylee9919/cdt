/* ========================================================================

   (C) Copyright 2025 by Seong Woo Lee, All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   ======================================================================== */


#define log_assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }

static int hex_to_int(char ch) {
    if (ch >= '0' && ch <= '9') { return ch - '0'; }
    if (ch >= 'A' && ch <= 'F') { return ch - 'A' + 10; }
    if (ch >= 'a' && ch <= 'f') { return ch - 'a' + 10; }
    return -1;
}

typedef struct {
    std::vector<float> floats;
    std::vector<int>   counts;
} Parse_Result;

Parse_Result float_array_from_log_file(const char *path) {
    Parse_Result result = {};

    FILE *file = fopen(path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t sz = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buf = (char *)malloc(sz);
        char *end = buf + sz;
        fread(buf, sz, 1, file);

        int cnt = 0;
        for (char *cursor = buf; cursor < end;) {
            char c = *cursor;
            if (c==' ' || c =='\r' || c =='\n' || c=='\t') {
                cursor += 1; 
            } else if (c==';') {
                result.counts.push_back(cnt);
                cnt = 0;
                cursor += 1; 
            } else {
                log_assert((c == '0') && *(cursor+1)=='x');
                cursor += 2;

                uint32_t integer = 0;
                for (char *back = cursor + 8; cursor < back; cursor+=1) {
                    int addend = hex_to_int(*cursor);
                    integer = ((integer<<4) + addend);
                }

                float fp = *((float *)&(integer));
                result.floats.push_back(fp);
                cnt += 1;
            }
        }

        fclose(file);
    } else {
        fprintf(stderr, "[Error] Could not open insert log.\n");
    }

    return result;
}
