// **tab, a nie tab[][]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#define LINE_BUF_SIZE 16384

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

int main(int argc, char **argv) {
    const char *filename1 = "mac1.txt";
    const char *filename2 = "mac2.txt";
    float *mat1 = NULL, *mat2 = NULL, *mat_wynik = NULL;
    int rows1 = 0, cols1 = 0, rows2 = 0, cols2 = 0;
    if (wczytaj_macierz(filename1, &mat1, &rows1, &cols1) != 0) { fprintf(stderr, "Blad wczytywania %s\n", filename1); return 1; }
    if (wczytaj_macierz(filename2, &mat2, &rows2, &cols2) != 0) { fprintf(stderr, "Blad wczytywania %s\n", filename2); free(mat1); return 1; }
    printf("MACIERZ 1:\n"); wypisz_macierz(mat1, rows1, cols1);
    printf("\nMACIERZ 2:\n"); wypisz_macierz(mat2, rows2, cols2);

    printf("\nWybierz operacje:\n1 - Dodawanie macierzy\n2 - Mnozenie macierzy\nTwoj wybor: ");
    int wybor = 0;
    if (scanf("%d", &wybor) != 1) { fprintf(stderr, "Blad odczytu wyboru!\n"); free(mat1); free(mat2); return 1; }

    if (wybor == 1) {
        if (rows1 != rows2 || cols1 != cols2) {
            fprintf(stderr, "Nie mozna dodac macierzy o roznych rozmiarach!\n");
            free(mat1); free(mat2); return 1;
        }
        mat_wynik = malloc((size_t)rows1 * cols1 * sizeof(float));
        if (!mat_wynik) { fprintf(stderr, "Brak pamieci na wynik\n"); free(mat1); free(mat2); return 1; }
        dodaj_macierze(mat1, mat2, mat_wynik, rows1, cols1);
        printf("\nSuma macierzy:\n"); wypisz_macierz(mat_wynik, rows1, cols1);
        free(mat_wynik);
    } else if (wybor == 2) {
        if (cols1 != rows2) {
            fprintf(stderr, "Nie mozna mnozyc: liczba kolumn macierzy 1 musi byc rowna liczbie wierszy macierzy 2!\n");
            free(mat1); free(mat2); return 1;
        }
        mat_wynik = malloc((size_t)rows1 * cols2 * sizeof(float));
        if (!mat_wynik) { fprintf(stderr, "Brak pamieci na wynik\n"); free(mat1); free(mat2); return 1; }
        mnoz_macierze(mat1, rows1, cols1, mat2, rows2, cols2, mat_wynik);
        printf("\nIloczyn macierzy:\n"); wypisz_macierz(mat_wynik, rows1, cols2);
        free(mat_wynik);
    } else {
        printf("Nieznana operacja.\n");
    }
    free(mat1); free(mat2);
    return 0;
}
