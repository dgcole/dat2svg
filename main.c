#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

struct dat_file {
    char** lines;
    int line_count;
};

void print_usage() {
    printf("usage: dat2svg file.dat\n");
}

struct dat_file read_dat(char* file) {
    FILE *dat_fp;
    errno_t err = fopen_s(&dat_fp, file, "r");
    if (err != 0) {
        fprintf(stderr, "Cannot open file '%s' : %s\n", file, strerror(err));

        struct dat_file empty = {NULL, 0};
        return empty;
    }

    fseek(dat_fp, 0L, SEEK_END);
    size_t dat_len = ftell(dat_fp);
    fseek(dat_fp, 0L, SEEK_SET);

    char * dat_buf = (char*) malloc(dat_len + 1);
    fread(dat_buf, sizeof(char), dat_len, dat_fp);
    dat_buf[dat_len] = '\0';

    unsigned int line_cnt = 1;
    for (long i = 0; i < dat_len; i++) {
        line_cnt += (dat_buf[i] == '\n');
    }

    char ** lines = (char**) malloc(sizeof(char*) * line_cnt);

    char * start_nxt = dat_buf;
    for (int i = 0; i < line_cnt; i++) {
        unsigned int line_len = 0;
        char * line_start_ptr = start_nxt;
        char * line_curr_ptr = line_start_ptr;

        while (*line_curr_ptr++ != '\n');

        line_len = (line_curr_ptr - line_start_ptr);

        char* line = (char*) malloc(line_len + 1);
        strncpy(line, line_start_ptr, line_len);
        line[line_len] = '\0';

        lines[i] = line;
        start_nxt = line_curr_ptr;
    }

    // Compact lines to skip blanks
    for (int i = 0; i < line_cnt - 1; i++) {
        char * curr_line = lines[i];
        bool empty = true;
        while (*curr_line != '\0') {
            if (!isspace(*curr_line)) {
                empty = false;
                break;
            }
            curr_line++;
        }

        if (empty) {
            free(lines[i]);
            for (int j = i; j < line_cnt - 1; j++) {
                lines[j] = lines[j + 1];
            }
            line_cnt--;
        }
    }

    free(dat_buf);
    fclose(dat_fp);

    struct dat_file dat = {lines, line_cnt};
    return dat;
}

void write_svg(char* file, struct dat_file dat) {
    int start_line = 0;
    for (int i = 0; i < dat.line_count; i++) {
        if (dat.lines[i][0] == '*') {
            start_line = i + 1;
            break;
        }
    }

    float lExt, rExt, bExt, tExt;
    bool hasExtents = true;
    if (sscanf(dat.lines[start_line], "%f %f %f %f", &lExt, &tExt, &rExt, &bExt) != 4) {
        lExt = 0;
        bExt = 0;

        rExt = 640;
        tExt = 640;

        hasExtents = false;
    }

    FILE*svg_fp;
    errno_t err = fopen_s(&svg_fp, file, "w+");
    if (err != 0) {
        fprintf(stderr, "Cannot open file '%s' : %s\n", file, strerror(err));
        return;
    }
    fprintf(svg_fp, "<svg viewBox=\"%f %f %f %f\" xmlns=\"http://www.w3.org/2000/svg\">\n", lExt, bExt, rExt - lExt, tExt - bExt);

    int polyline_cnt;
    sscanf(dat.lines[hasExtents ? start_line + 1 : start_line], "%d", &polyline_cnt);

    int curr_line = hasExtents ? start_line + 2 : start_line + 1;
    for (int i = 0; i < polyline_cnt; i++) {
        int polyline_len;
        sscanf(dat.lines[curr_line], "%d", &polyline_len);
        for (int j = curr_line + 1; j < (curr_line + polyline_len); j++) {
            float x0, y0, x1, y1;
            sscanf(dat.lines[j], " %f %f", &x0, &y0);
            sscanf(dat.lines[j + 1], " %f %f", &x1, &y1);

            float height = tExt - bExt;
            y0 = bExt + (tExt - y0);
            y1 = bExt + (tExt - y1);

            fprintf(svg_fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"#000000\" stroke-width=\".1%%\" />\n", x0, y0, x1, y1);
        }

        curr_line += polyline_len + 1;
    }

    fprintf(svg_fp, "</svg>\n");
    fclose(svg_fp);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage();
        return 0;
    }

    struct dat_file dat = read_dat(argv[1]);
    if (dat.lines == NULL) {
        return 1;
    }

    char * out_filename = (char*) malloc(strlen(argv[1]) + 5);

    char * in_name_ptr = argv[1];
    char * out_name_ptr = out_filename;
    while (*in_name_ptr != '.') {
        *out_name_ptr++ = *in_name_ptr++;
    }
    strcpy(out_name_ptr, ".svg\0");

    write_svg(out_filename, dat);

    free(out_filename);
    for (int i = 0; i < dat.line_count; i++) {
        free(dat.lines[i]);
    }
    free(dat.lines);

    return 0;
}
