// **tab, a nie tab[][]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#define LINE_BUF_SIZE 16384

typedef struct { float re, im; } Complex;

// Deklaracje funkcji dla macierzy zespolonych
int wczytaj_macierz_complex(const char *nazwa_pliku, Complex **pmat, int *prows, int *pcols);
void wypisz_macierz_complex(const Complex *mat, int rows, int cols);
void dodaj_macierze_complex(const Complex *a, const Complex *b, Complex *wynik, int rows, int cols);
void odejmij_macierze_complex(const Complex *a, const Complex *b, Complex *wynik, int rows, int cols);
void mnoz_macierze_complex(const Complex *a, int a_rows, int a_cols, const Complex *b, int b_rows, int b_cols, Complex *wynik);

static int is_blank_line(const char *s) {
    while (*s) {
        if (*s != '\n' && *s != '\r' && *s != ' ' && *s != '\t') return 0;
        s++;
    }
    return 1;
}

static void trim_whitespace(char *s) {
    // trim leading
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    // trim trailing
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

// Parsowanie liczby zespolonej w formacie a+b i lub a-b i
int parse_complex(const char *str, Complex *out) {
    float re = 0, im = 0;
    char *i_ptr = strchr(str, 'i');
    if (!i_ptr) {
        // tylko rzeczywista
        if (sscanf(str, "%f", &re) == 1) {
            out->re = re; out->im = 0;
            return 1;
        } else return 0;
    }
    char buf[LINE_BUF_SIZE];
    strncpy(buf, str, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    buf[i_ptr-str] = 0;
    char *plus = strrchr(buf, '+');
    char *minus = strrchr(buf, '-');
    if (plus && plus != buf) {
        *plus = 0;
        if (sscanf(buf, "%f", &re) != 1) return 0;
        if (sscanf(plus+1, "%f", &im) != 1) im = 1;
    } else if (minus && minus != buf) {
        *minus = 0;
        if (sscanf(buf, "%f", &re) != 1) return 0;
        if (sscanf(minus, "%f", &im) != 1) im = -1;
    } else {
        // tylko czesc urojona
        if (sscanf(buf, "%f", &im) != 1) im = 1;
    }
    out->re = re; out->im = im;
    return 1;
}

void wczytaj_macierz_z_pliku_txt(const char *nazwa_pliku, int *tab) {
    FILE *fp = fopen(nazwa_pliku, "r");
    if (fp == NULL) {
        printf("Nie mozna otworzyc pliku %s\n", nazwa_pliku);
        return;
    }

    char line[LINE_BUF_SIZE];
    int rows = 0, cols = 0;
    int found_size = 0;

    // Wczytaj rozmiar macierzy z pierwszej niepustej linii
    while (fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line;
        char *end;
        rows = (int)strtol(p, &end, 10);
        if (end == p) {
            fprintf(stderr, "Blad: nieprawidlowy format rozmiaru macierzy (wiersze)\n");
            fclose(fp);
            return;
        }
        p = end;
        while (*p == '\t' || isspace((unsigned char)*p)) p++;
        cols = (int)strtol(p, &end, 10);
        if (end == p) {
            fprintf(stderr, "Blad: nieprawidlowy format rozmiaru macierzy (kolumny)\n");
            fclose(fp);
            return;
        }
        found_size = 1;
        break;
    }

    if (!found_size || rows <= 0 || cols <= 0) {
        fprintf(stderr, "Blad: nieprawidlowy rozmiar macierzy\n");
        fclose(fp);
        return;
    }

    float *mat = malloc((size_t)rows * cols * sizeof(float));
    if (!mat) {
        fprintf(stderr, "Blad: brak pamieci\n");
        fclose(fp);
        return;
    }

    int r = 0;
    // Wczytaj dane macierzy
    while (r < rows && fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line;
        for (int c = 0; c < cols; ++c) {
            char *start = p;
            char *end = start;
            while (*end && *end != '\t' && *end != '\n' && *end != '\r') end++;
            size_t toklen = (size_t)(end - start);
            char token[LINE_BUF_SIZE];
            if (toklen >= sizeof(token)) toklen = sizeof(token) - 1;
            memcpy(token, start, toklen);
            token[toklen] = '\0';
            trim_whitespace(token);

            if (token[0] == '\0') {
                fprintf(stderr, "Blad: pusty element w wierszu %d, kolumnie %d\n", r + 1, c + 1);
                free(mat);
                fclose(fp);
                return;
            }

            errno = 0;
            char *endptr = NULL;
            float val = strtof(token, &endptr);
            while (*endptr && isspace((unsigned char)*endptr)) endptr++;
            if (endptr == token || *endptr != '\0') {
                fprintf(stderr, "Blad parsowania liczby w wierszu %d, kolumnie %d: '%s'\n", r + 1, c + 1, token);
                free(mat);
                fclose(fp);
                return;
            }
            if (errno == ERANGE || !isfinite((double)val)) {
                fprintf(stderr, "Blad: wartosc poza zakresem typu float w wierszu %d, kolumnie %d: '%s'\n", r + 1, c + 1, token);
                free(mat);
                fclose(fp);
                return;
            }
            mat[r * cols + c] = val;
            p = end;
            if (*p == '\t') p++;
        }
        r++;
    }

    if (r < rows) {
        fprintf(stderr, "Blad: za malo wierszy danych (oczekiwano %d, znaleziono %d)\n", rows, r);
        free(mat);
        fclose(fp);
        return;
    }

    fclose(fp);

    printf("Macierz poprawna. Rozmiar: %d wierszy x %d kolumn\n", rows, cols);
    printf("Zawartosc macierzy:\n");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            printf("%g", mat[i * cols + j]);
            if (j + 1 < cols) printf("\t");
        }
        printf("\n");
    }

    free(mat);
}

// Wczytaj macierz do bufora, zwróć 0 jeśli OK
int wczytaj_macierz(const char *nazwa_pliku, float **pmat, int *prows, int *pcols) {
    FILE *fp = fopen(nazwa_pliku, "r");
    if (!fp) {
        fprintf(stderr, "Nie mozna otworzyc pliku %s\n", nazwa_pliku);
        return 1;
    }
    char line[LINE_BUF_SIZE];
    int rows = 0, cols = 0, found_size = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line, *end;
        rows = (int)strtol(p, &end, 10);
        if (end == p) { fclose(fp); return 2; }
        p = end; while (*p == '\t' || isspace((unsigned char)*p)) p++;
        cols = (int)strtol(p, &end, 10);
        if (end == p) { fclose(fp); return 2; }
        found_size = 1; break;
    }
    if (!found_size || rows <= 0 || cols <= 0) { fclose(fp); return 3; }
    float *mat = malloc((size_t)rows * cols * sizeof(float));
    if (!mat) { fclose(fp); return 4; }
    int r = 0;
    while (r < rows && fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line;
        for (int c = 0; c < cols; ++c) {
            char *start = p, *end = start;
            while (*end && *end != '\t' && *end != '\n' && *end != '\r') end++;
            size_t toklen = (size_t)(end - start);
            char token[LINE_BUF_SIZE];
            if (toklen >= sizeof(token)) toklen = sizeof(token) - 1;
            memcpy(token, start, toklen); token[toklen] = '\0';
            trim_whitespace(token);
            if (token[0] == '\0') { free(mat); fclose(fp); return 5; }
            errno = 0; char *endptr = NULL;
            float val = strtof(token, &endptr);
            while (*endptr && isspace((unsigned char)*endptr)) endptr++;
            if (endptr == token || *endptr != '\0') { free(mat); fclose(fp); return 6; }
            if (errno == ERANGE || !isfinite((double)val)) { free(mat); fclose(fp); return 7; }
            mat[r * cols + c] = val;
            p = end; if (*p == '\t') p++;
        }
        r++;
    }
    fclose(fp);
    if (r < rows) { free(mat); return 8; }
    *pmat = mat; *prows = rows; *pcols = cols;
    return 0;
}

void wypisz_macierz(const float *mat, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            printf("%g", mat[i * cols + j]);
            if (j + 1 < cols) printf("\t");
        }
        printf("\n");
    }
}

int dodaj_macierze(const float *a, const float *b, float *wynik, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i) wynik[i] = a[i] + b[i];
    return 0;
}

int odejmij_macierze(const float *a, const float *b, float *wynik, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i) wynik[i] = a[i] - b[i];
    return 0;
}

int mnoz_macierze(const float *a, int a_rows, int a_cols, const float *b, int b_rows, int b_cols, float *wynik) {
    if (a_cols != b_rows) return 1;
    for (int i = 0; i < a_rows; ++i) {
        for (int j = 0; j < b_cols; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < a_cols; ++k) {
                sum += a[i * a_cols + k] * b[k * b_cols + j];
            }
            wynik[i * b_cols + j] = sum;
        }
    }
    return 0;
}

// --- Funkcje dla macierzy zespolonych ---
int wczytaj_macierz_complex(const char *nazwa_pliku, Complex **pmat, int *prows, int *pcols) {
    FILE *fp = fopen(nazwa_pliku, "r");
    if (!fp) { fprintf(stderr, "Nie mozna otworzyc pliku %s\n", nazwa_pliku); return 1; }
    char line[LINE_BUF_SIZE];
    int rows = 0, cols = 0, found_size = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line, *end;
        rows = (int)strtol(p, &end, 10);
        if (end == p) { fclose(fp); return 2; }
        p = end; while (*p == '\t' || isspace((unsigned char)*p)) p++;
        cols = (int)strtol(p, &end, 10);
        if (end == p) { fclose(fp); return 2; }
        found_size = 1; break;
    }
    if (!found_size || rows <= 0 || cols <= 0) { fclose(fp); return 3; }
    Complex *mat = malloc((size_t)rows * cols * sizeof(Complex));
    if (!mat) { fclose(fp); return 4; }
    int r = 0;
    while (r < rows && fgets(line, sizeof(line), fp)) {
        if (is_blank_line(line)) continue;
        char *p = line;
        for (int c = 0; c < cols; ++c) {
            char *start = p, *end = start;
            while (*end && *end != '\t' && *end != '\n' && *end != '\r') end++;
            size_t toklen = (size_t)(end - start);
            char token[LINE_BUF_SIZE];
            if (toklen >= sizeof(token)) toklen = sizeof(token) - 1;
            memcpy(token, start, toklen); token[toklen] = '\0';
            trim_whitespace(token);
            if (token[0] == '\0') { free(mat); fclose(fp); return 5; }
            Complex val;
            if (!parse_complex(token, &val)) { free(mat); fclose(fp); return 6; }
            mat[r * cols + c] = val;
            p = end; if (*p == '\t') p++;
        }
        r++;
    }
    fclose(fp);
    if (r < rows) { free(mat); return 8; }
    *pmat = mat; *prows = rows; *pcols = cols;
    return 0;
}

void wypisz_macierz_complex(const Complex *mat, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            printf("%.2f%+.2fi", mat[i * cols + j].re, mat[i * cols + j].im);
            if (j + 1 < cols) printf("\t");
        }
        printf("\n");
    }
}

void dodaj_macierze_complex(const Complex *a, const Complex *b, Complex *wynik, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i) {
        wynik[i].re = a[i].re + b[i].re;
        wynik[i].im = a[i].im + b[i].im;
    }
}
void odejmij_macierze_complex(const Complex *a, const Complex *b, Complex *wynik, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i) {
        wynik[i].re = a[i].re - b[i].re;
        wynik[i].im = a[i].im - b[i].im;
    }
}
void mnoz_macierze_complex(const Complex *a, int a_rows, int a_cols, const Complex *b, int b_rows, int b_cols, Complex *wynik) {
    for (int i = 0; i < a_rows; ++i) {
        for (int j = 0; j < b_cols; ++j) {
            Complex sum = {0,0};
            for (int k = 0; k < a_cols; ++k) {
                float ar = a[i*a_cols+k].re, ai = a[i*a_cols+k].im;
                float br = b[k*b_cols+j].re, bi = b[k*b_cols+j].im;
                sum.re += ar*br - ai*bi;
                sum.im += ar*bi + ai*br;
            }
            wynik[i*b_cols+j] = sum;
        }
    }
}

// --- Deklaracje funkcji pomocniczych ---
void transpose_float(const float *src, float *dst, int rows, int cols);
void transpose_complex(const Complex *src, Complex *dst, int rows, int cols);
int save_matrix_float(const char *filename, const float *mat, int rows, int cols);
int save_matrix_complex(const char *filename, const Complex *mat, int rows, int cols);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Uzycie:\n");
        printf("  %s mac1.txt + mac2.txt [wynik.txt]\n", argv[0]);
        printf("  %s mac1.txt - mac2.txt [wynik.txt]\n", argv[0]);
        printf("  %s mac1.txt * mac2.txt [wynik.txt]\n", argv[0]);
        printf("  %s mac1.txt ^ [wynik.txt]\n", argv[0]);
        printf("  %s mac2.txt ^ [wynik.txt]\n", argv[0]);
        return 1;
    }
    // Transpozycja
    if ((argc == 3 || argc == 4) && strcmp(argv[2], "^") == 0) {
        const char *infile = argv[1];
        int is_complex = 0;
        FILE *f = fopen(infile, "r");
        char buf[LINE_BUF_SIZE];
        if (f) { while (fgets(buf, sizeof(buf), f)) { if (strchr(buf, 'i')) { is_complex = 1; break; } } fclose(f); }
        if (is_complex) {
            Complex *mat = NULL, *matT = NULL;
            int rows = 0, cols = 0;
            if (wczytaj_macierz_complex(infile, &mat, &rows, &cols) != 0) { fprintf(stderr, "Blad wczytywania %s\n", infile); return 1; }
            matT = malloc((size_t)rows * cols * sizeof(Complex));
            transpose_complex(mat, matT, rows, cols);
            printf("Transpozycja macierzy:\n"); wypisz_macierz_complex(matT, cols, rows);
            if (argc == 4) save_matrix_complex(argv[3], matT, cols, rows);
            free(mat); free(matT);
        } else {
            float *mat = NULL, *matT = NULL;
            int rows = 0, cols = 0;
            if (wczytaj_macierz(infile, &mat, &rows, &cols) != 0) { fprintf(stderr, "Blad wczytywania %s\n", infile); return 1; }
            matT = malloc((size_t)rows * cols * sizeof(float));
            transpose_float(mat, matT, rows, cols);
            printf("Transpozycja macierzy:\n"); wypisz_macierz(matT, cols, rows);
            if (argc == 4) save_matrix_float(argv[3], matT, cols, rows);
            free(mat); free(matT);
        }
        return 0;
    }
    // Operacje binarne: + - *
    if (argc < 4) { fprintf(stderr, "Za malo argumentow!\n"); return 1; }
    const char *file1 = argv[1];
    const char *op = argv[2];
    const char *file2 = argv[3];
    const char *outfile = (argc >= 5) ? argv[4] : NULL;
    int is_complex = 0;
    FILE *f = fopen(file1, "r");
    char buf[LINE_BUF_SIZE];
    if (f) { while (fgets(buf, sizeof(buf), f)) { if (strchr(buf, 'i')) { is_complex = 1; break; } } fclose(f); }
    if (is_complex) {
        Complex *mat1 = NULL, *mat2 = NULL, *mat_wynik = NULL;
        int rows1 = 0, cols1 = 0, rows2 = 0, cols2 = 0;
        if (wczytaj_macierz_complex(file1, &mat1, &rows1, &cols1) != 0) { fprintf(stderr, "Blad wczytywania %s\n", file1); return 1; }
        if (wczytaj_macierz_complex(file2, &mat2, &rows2, &cols2) != 0) { fprintf(stderr, "Blad wczytywania %s\n", file2); free(mat1); return 1; }
        if (strcmp(op, "+") == 0) {
            if (rows1 != rows2 || cols1 != cols2) { fprintf(stderr, "Nie mozna dodac macierzy o roznych rozmiarach!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols1 * sizeof(Complex));
            dodaj_macierze_complex(mat1, mat2, mat_wynik, rows1, cols1);
            printf("Suma macierzy:\n"); wypisz_macierz_complex(mat_wynik, rows1, cols1);
            if (outfile) save_matrix_complex(outfile, mat_wynik, rows1, cols1);
        } else if (strcmp(op, "-") == 0) {
            if (rows1 != rows2 || cols1 != cols2) { fprintf(stderr, "Nie mozna odjac macierzy o roznych rozmiarach!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols1 * sizeof(Complex));
            odejmij_macierze_complex(mat1, mat2, mat_wynik, rows1, cols1);
            printf("Roznica macierzy:\n"); wypisz_macierz_complex(mat_wynik, rows1, cols1);
            if (outfile) save_matrix_complex(outfile, mat_wynik, rows1, cols1);
        } else if (strcmp(op, "*") == 0) {
            if (cols1 != rows2) { fprintf(stderr, "Nie mozna mnozyc: liczba kolumn macierzy 1 musi byc rowna liczbie wierszy macierzy 2!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols2 * sizeof(Complex));
            mnoz_macierze_complex(mat1, rows1, cols1, mat2, rows2, cols2, mat_wynik);
            printf("Iloczyn macierzy:\n"); wypisz_macierz_complex(mat_wynik, rows1, cols2);
            if (outfile) save_matrix_complex(outfile, mat_wynik, rows1, cols2);
        } else { fprintf(stderr, "Nieznana operacja: %s\n", op); free(mat1); free(mat2); return 1; }
        free(mat1); free(mat2); free(mat_wynik);
    } else {
        float *mat1 = NULL, *mat2 = NULL, *mat_wynik = NULL;
        int rows1 = 0, cols1 = 0, rows2 = 0, cols2 = 0;
        if (wczytaj_macierz(file1, &mat1, &rows1, &cols1) != 0) { fprintf(stderr, "Blad wczytywania %s\n", file1); return 1; }
        if (wczytaj_macierz(file2, &mat2, &rows2, &cols2) != 0) { fprintf(stderr, "Blad wczytywania %s\n", file2); free(mat1); return 1; }
        if (strcmp(op, "+") == 0) {
            if (rows1 != rows2 || cols1 != cols2) { fprintf(stderr, "Nie mozna dodac macierzy o roznych rozmiarach!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols1 * sizeof(float));
            dodaj_macierze(mat1, mat2, mat_wynik, rows1, cols1);
            printf("Suma macierzy:\n"); wypisz_macierz(mat_wynik, rows1, cols1);
            if (outfile) save_matrix_float(outfile, mat_wynik, rows1, cols1);
        } else if (strcmp(op, "-") == 0) {
            if (rows1 != rows2 || cols1 != cols2) { fprintf(stderr, "Nie mozna odjac macierzy o roznych rozmiarach!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols1 * sizeof(float));
            odejmij_macierze(mat1, mat2, mat_wynik, rows1, cols1);
            printf("Roznica macierzy:\n"); wypisz_macierz(mat_wynik, rows1, cols1);
            if (outfile) save_matrix_float(outfile, mat_wynik, rows1, cols1);
        } else if (strcmp(op, "*") == 0) {
            if (cols1 != rows2) { fprintf(stderr, "Nie mozna mnozyc: liczba kolumn macierzy 1 musi byc rowna liczbie wierszy macierzy 2!\n"); free(mat1); free(mat2); return 1; }
            mat_wynik = malloc((size_t)rows1 * cols2 * sizeof(float));
            mnoz_macierze(mat1, rows1, cols1, mat2, rows2, cols2, mat_wynik);
            printf("Iloczyn macierzy:\n"); wypisz_macierz(mat_wynik, rows1, cols2);
            if (outfile) save_matrix_float(outfile, mat_wynik, rows1, cols2);
        } else { fprintf(stderr, "Nieznana operacja: %s\n", op); free(mat1); free(mat2); return 1; }
        free(mat1); free(mat2); free(mat_wynik);
    }
    return 0;
}

// --- Implementacje funkcji pomocniczych ---
void transpose_float(const float *src, float *dst, int rows, int cols) {
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            dst[j * rows + i] = src[i * cols + j];
}
void transpose_complex(const Complex *src, Complex *dst, int rows, int cols) {
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            dst[j * rows + i] = src[i * cols + j];
}
int save_matrix_float(const char *filename, const float *mat, int rows, int cols) {
    FILE *f = fopen(filename, "w");
    if (!f) return 1;
    fprintf(f, "%d\t%d\n", rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            fprintf(f, "%g", mat[i * cols + j]);
            if (j + 1 < cols) fprintf(f, "\t");
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}
int save_matrix_complex(const char *filename, const Complex *mat, int rows, int cols) {
    FILE *f = fopen(filename, "w");
    if (!f) return 1;
    fprintf(f, "%d\t%d\n", rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            fprintf(f, "%.2f%+.2fi", mat[i * cols + j].re, mat[i * cols + j].im);
            if (j + 1 < cols) fprintf(f, "\t");
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}
