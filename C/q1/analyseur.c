/*
 * Question 1 : Analyseur syntaxique pour les polynomes.
 *
 * Grammaire reconnue (BNF) :
 *   polynome   -> [-] monome { (+|-) monome }
 *   monome     -> nombre * xpuissance | xpuissance | nombre
 *   xpuissance -> X | X^naturel
 *   naturel    -> chiffre {chiffre}
 *   nombre     -> naturel [.{chiffre}]
 *   chiffre    -> 0|1|2|3|4|5|6|7|8|9
 *
 * Le programme lit l'entree standard caractere par caractere et
 * verifie qu'elle respecte la grammaire ci-dessus. En cas d'erreur,
 * il affiche un message sur stderr et arrete le programme avec un
 * code d'erreur. Sinon, il affiche "polynome reconnu".
 *
 * Les espaces et tabulations sont ignores entre tokens : on accepte
 * indifferemment "4*X^2" et "4 * X ^ 2". En revanche, on les
 * interdit a l'interieur d'un nombre : "3 5" doit etre rejete (et
 * non traite comme "35"). Pour cela, lire() saute les espaces, mais
 * la lecture des chiffres consecutifs utilise getchar() direct.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Caractere courant : celui qu'on est en train d'examiner.
   On stocke un int (et non un char) pour pouvoir comparer a EOF. */
static int car;

/* Avance d'un caractere dans 'car', en sautant les espaces et
   tabulations rencontres au passage. Apres l'appel, 'car' contient
   donc soit un caractere significatif, soit '\n', soit EOF. */
static void lire(void) {
    do {
        car = getchar();
    } while (car == ' ' || car == '\t');
}

/* Affiche un message d'erreur sur stderr et arrete le programme. */
static void erreur(const char *msg) {
    fprintf(stderr, "Erreur de syntaxe : %s\n", msg);
    exit(EXIT_FAILURE);
}

/* Prototypes pour permettre l'appel mutuel entre les fonctions. */
static void polynome(void);
static void monome(void);
static void xpuissance(void);
static void nombre(void);
static void naturel(void);

/* naturel -> chiffre {chiffre}
   Lit un entier non signe : au moins un chiffre, suivi de zero ou
   plusieurs autres chiffres. On utilise getchar() direct (et non
   lire()) pour interdire les espaces entre les chiffres : "3 5" ne
   doit PAS etre accepte comme "35". A la fin, on saute les espaces
   pour positionner 'car' sur le prochain caractere significatif. */
static void naturel(void) {
    if (!isdigit(car))
        erreur("chiffre attendu");
    do {
        car = getchar();
    } while (isdigit(car));
    while (car == ' ' || car == '\t')
        car = getchar();
}

/* nombre -> naturel [.{chiffre}]
   Lit un naturel, eventuellement suivi d'un point et de chiffres.
   Pas d'espace autorise autour du point ni entre les chiffres
   decimaux. */
static void nombre(void) {
    naturel();
    if (car == '.') {
        /* Lecture brute du caractere suivant le point : on n'autorise
           pas un espace entre '.' et les chiffres decimaux. */
        car = getchar();
        while (isdigit(car))
            car = getchar();
        /* Une fois la partie decimale lue, on saute les espaces. */
        while (car == ' ' || car == '\t')
            car = getchar();
    }
}

/* xpuissance -> X | X^naturel
   Lit la lettre X, eventuellement suivie de ^ et d'un naturel
   (l'exposant). */
static void xpuissance(void) {
    if (car != 'X')
        erreur("'X' attendu");
    lire();
    if (car == '^') {
        lire();
        naturel();
    }
}

/* monome -> nombre * xpuissance | xpuissance | nombre
   On choisit la bonne regle selon le caractere courant :
     - si 'X' : c'est une xpuissance seule
     - si chiffre : c'est un nombre, eventuellement suivi de
       '* xpuissance'
     - sinon : erreur. */
static void monome(void) {
    if (car == 'X') {
        xpuissance();
    } else if (isdigit(car)) {
        nombre();
        if (car == '*') {
            lire();
            xpuissance();
        }
        /* Sinon, c'est un monome reduit a un nombre : rien a faire. */
    } else {
        erreur("debut de monome attendu (X ou chiffre)");
    }
}

/* polynome -> [-] monome { (+|-) monome }
   Un polynome est une suite non vide de monomes separes par '+' ou
   '-', le tout pouvant etre precede d'un signe '-'. */
static void polynome(void) {
    if (car == '-')
        lire();
    monome();
    while (car == '+' || car == '-') {
        lire();
        monome();
    }
}

int main(void) {
    /* Amorcage : on charge le premier caractere avant l'analyse. */
    lire();
    polynome();
    /* Apres le polynome, on doit etre en fin de ligne ou de fichier ;
       tout autre caractere est une erreur. */
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le polynome");
    printf("polynome reconnu\n");
    return 0;
}
