/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

typedef struct {
    float *floats;
    int   *counts;
} Parse_Result;

int hex_to_int(char ch);
Parse_Result float_array_from_log_file(const char *path);
