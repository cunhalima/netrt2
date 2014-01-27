#include <features.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#define __USE_BSD
#include <termios.h>
#undef __USE_BSD
#include <unistd.h>
#include <sys/select.h>
#include "defs.h"

static szb_t *kbd_buf;
static struct termios orig_termios;
static struct termios new_termios;
static bool input_on;
static int tty_erase;
static bool quitFlag;
static cmdparserfn_t cmdparserfn = NULL;

static void set_raw(bool raw) {                                 // coloca o console no modo cru (ou desativa o modo cru)
    assert(input_on);
    if (raw) {
        tcsetattr(0, TCSANOW, &new_termios);
    } else {
        tcsetattr(0, TCSANOW, &orig_termios);
    }
}

static void init_term() {                                       // prepara a utilização do modo cru
    assert(input_on);
    tcgetattr(0, &orig_termios);
    tty_erase = orig_termios.c_cc[VERASE];
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    cfmakeraw(&new_termios);
}

static int kbhit() {                                            // detecta se uma tecla foi pressionada no modo cru
    fd_set fds;
    struct timeval tv = { 0L, 0L };

    assert(input_on);
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

static int getch() {                                            // pega a tecla pressionada no modo cru
    int r;
    unsigned char c;

    assert(input_on);
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

/* ====================================== */
bool con_init() {                                               // inicializa o teclado no modo assíncrono
    input_on = true;
    kbd_buf = szb_create();
    init_term();
    if (input_on) {
        set_raw(true);
    }
    quitFlag = false;
    return true;
}

bool con_cleanup() {                                            // libera o modo assíncrono
    if (input_on) {
        set_raw(false);
    }
    if (kbd_buf != NULL) {
        szb_free(kbd_buf);
        kbd_buf = NULL;
    }
    return true;
}

void con_print(const char *msg) {                           // toda vez que imprimimos na tela, temos que desativar o modo cru antes
    if (input_on) {
        set_raw(false);
    }
    printf("%s", msg);
    fflush(stdout);
    if (input_on) {                                         // reativar o modo cru após a imprssão na tela
        set_raw(true);
    }
}

void con_printf(const char *fmt, ...) {                     // nosso printf
    char text[1024];
    va_list ap;
    if (fmt == NULL)
        return;      
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);    
    con_print(text);
}

void con_error(const char *msg) {                           // msg de erro (NÃO LEMBRO ONDE USA)
    if (input_on) {
        set_raw(false);
    }
    fprintf(stderr, "[ERROR] %s", msg);
    fflush(stderr);
    if (input_on) {
        set_raw(true);
    }
}

void con_errorf(const char *fmt, ...) {                 // erro formatado
    char text[1024];
    va_list ap;
    if (fmt == NULL)
        return;      
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);    
    con_error(text);
}

static void con_run(char *str) {                        // faz o parseamento inicial da linha de comando
    int len;
    int pos;

    while(isspace(*str)) {
        str++;
    }
    if (*str == '#') {
        return;
    }
    len = strlen(str);
    pos = len - 1;
    while(pos > 0) {
        if (!isspace(str[pos])) {
            break;
        }
        str[pos] = '\0';
    }
    if (strcmp(str, "/quit") == 0) {
        quitFlag = true;
        return;
    }
    if (cmdparserfn != NULL) {
        cmdparserfn(str);
    }
}

void con_checkInput() {                 // verifica se algo foi pressionado no teclado (nao bloqueante)
    if (!input_on) {
        return;
    }
    while (kbhit()) {                   // faz esse lanço enquanto algo estiver sendo pressionado
        int r;
        char cc;
        //char outro;

        cc = getch();
        if (cc == tty_erase) {                  // processa o backspace
            int len = szb_getSize(kbd_buf);
            if (len > 0) {
                r = write(1, "\b \b", 3);
                szb_resize(kbd_buf, len - 1);
            }
            continue;
        }
        //outro = 'a';
        if (cc == 27) {                         // processa o ESC
            quitFlag = true;
            cc = 0;
        }
        if (cc < ' ') {                         // processa teclas especiais
            if (cc != '\r' && cc != '\t') {
                cc = 0;
            }
        }
        if (cc != 0) {                          // se tiver algum caractere imprimível, mostra ele na tela
            r = write(1, &cc, 1);
            if (cc == '\r') {
                cc = '\n';
                r = write(1, &cc, 1);           // aqui que efetivamente mostra na tela
            }
            if (cc != '\n') {
                szb_write8(kbd_buf, cc);        // e também guarda ele no nosso buffer
            }
        }
        if (cc == '\n') {                       // se a pessoa apertar ENTER, processa a linha de comando
            szb_write8(kbd_buf, '\0');
            con_run(szb_getPtr(kbd_buf));
            szb_resize(kbd_buf, 0);             // após processar, limpa o buffer
        }
    }
}

bool con_isRunning() {                          // verifica se o usuário não quis sair ainda
    return !quitFlag;
}

void con_register(cmdparserfn_t fn) {           // registra uma função de parseamento de comando
    cmdparserfn = fn;
}

void con_runfile(const char *path) {            // executa um script num arquivo
    FILE *file;
    char line[256];

    file = fopen(path, "rt");
    if (file == NULL) {
        con_errorf("unable to open %s\n", path);
        return;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        con_run(line);
    }
    fclose(file);
}
