/*
 * Question 2 : Codage des polynomes en memoire.
 *
 * Ce fichier reprend l'analyseur syntaxique de la Q1 et lui AJOUTE
 * (sans rien retirer) le code necessaire pour construire en memoire
 * une representation du polynome reconnu, sous forme d'une liste
 * chainee de maillons (coef, expo).
 *
 *   polynome   -> [-] monome { (+|-) monome }
 *   monome     -> nombre * xpuissance | xpuissance | nombre
 *   xpuissance -> X | X^naturel
 *   naturel    -> chiffre {chiffre}
 *   nombre     -> naturel [.{chiffre}]
 *
 * A la Q2, l'ordre des maillons dans la liste reflete simplement
 * l'ordre de lecture (le tri par degre decroissant viendra a la Q4).
 *
 * Le polynome nul est represente par une liste vide (NULL).
 *
 * Pour pouvoir verifier visuellement que la liste construite est
 * correcte, on a ajoute une fonction de debug 'afficheDebug' qui
 * affiche la liste sous forme brute "(coef, expo) -> ..." (la
 * fonction d'affichage propre du polynome viendra a la Q3).
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* ------------------------------------------------------------------
   Structure de donnees : un polynome est une liste chainee de
   maillons. Chaque maillon contient un coefficient (reel), un
   exposant (entier naturel) et un pointeur vers le maillon suivant.
   ------------------------------------------------------------------ */

typedef struct maillon {
    double          coef;
    int             expo;
    struct maillon *suivant;
} MAILLON;

typedef MAILLON *POINTEUR;

/* Alloue un nouveau maillon avec les valeurs donnees. En cas d'echec
   d'allocation, on arrete le programme. */
static POINTEUR nouveauMaillon(double coef, int expo, POINTEUR suivant) {
    POINTEUR m = (POINTEUR) malloc(sizeof(MAILLON));
    if (m == NULL) {
        fprintf(stderr, "Erreur : memoire insuffisante\n");
        exit(EXIT_FAILURE);
    }
    m->coef = coef;
    m->expo = expo;
    m->suivant = suivant;
    return m;
}

/* ------------------------------------------------------------------
   Outils de lecture (identiques a la Q1).
   ------------------------------------------------------------------ */

static int car;

static void lire(void) {
    do {
        car = getchar();
    } while (car == ' ' || car == '\t');
}

static void erreur(const char *msg) {
    fprintf(stderr, "Erreur de syntaxe : %s\n", msg);
    exit(EXIT_FAILURE);
}

/* ------------------------------------------------------------------
   Analyseur a descente recursive : chaque fonction reconnait son
   non-terminal ET renvoie la valeur reconnue (un nombre, un
   exposant, ou un maillon / un debut de liste).
   ------------------------------------------------------------------ */

static POINTEUR polynome(void);
static POINTEUR monome(void);
static int      xpuissance(void);
static double   nombre(void);
static int      naturel(void);

/* naturel -> chiffre {chiffre}
   Lit un entier non signe et renvoie sa valeur numerique.
   Pas d'espace autorise entre les chiffres. */
static int naturel(void) {
    if (!isdigit(car))
        erreur("chiffre attendu");
    int n = 0;
    do {
        n = n * 10 + (car - '0');
        car = getchar();
    } while (isdigit(car));
    /* On positionne 'car' sur le prochain caractere significatif. */
    while (car == ' ' || car == '\t')
        car = getchar();
    return n;
}

/* nombre -> naturel [.{chiffre}]
   Lit un nombre eventuellement decimal et renvoie sa valeur en
   double. */
static double nombre(void) {
    double valeur = (double) naturel();
    if (car == '.') {
        /* Lecture brute pour interdire un espace apres le point. */
        car = getchar();
        double facteur = 0.1;
        while (isdigit(car)) {
            valeur += (car - '0') * facteur;
            facteur /= 10.0;
            car = getchar();
        }
        while (car == ' ' || car == '\t')
            car = getchar();
    }
    return valeur;
}

/* xpuissance -> X | X^naturel
   Lit la lettre X (suivie ou non de ^naturel) et renvoie l'exposant.
   Cas "X" tout seul : exposant 1. */
static int xpuissance(void) {
    if (car != 'X')
        erreur("'X' attendu");
    lire();
    if (car == '^') {
        lire();
        return naturel();
    }
    return 1;
}

/* monome -> nombre * xpuissance | xpuissance | nombre
   Reconnait un monome et renvoie un nouveau maillon (coef, expo).
   Le coefficient est toujours POSITIF a ce stade : la gestion du
   signe est faite par polynome() qui l'applique apres coup. */
static POINTEUR monome(void) {
    if (car == 'X') {
        /* Forme : xpuissance seule -> coefficient implicite 1. */
        int e = xpuissance();
        return nouveauMaillon(1.0, e, NULL);
    }
    if (isdigit(car)) {
        double n = nombre();
        if (car == '*') {
            /* Forme : nombre * xpuissance. */
            lire();
            int e = xpuissance();
            return nouveauMaillon(n, e, NULL);
        }
        /* Forme : nombre tout seul -> exposant 0 (constante). */
        return nouveauMaillon(n, 0, NULL);
    }
    erreur("debut de monome attendu (X ou chiffre)");
    return NULL; /* inatteignable, juste pour faire taire le compilateur */
}

/* polynome -> [-] monome { (+|-) monome }
   Construit la liste chainee des monomes en respectant les signes.
   On accumule les maillons en fin de liste pour preserver l'ordre
   de lecture (ce qui rend la verification visuelle plus facile). */
static POINTEUR polynome(void) {
    int signe = 1;
    if (car == '-') {
        signe = -1;
        lire();
    }
    POINTEUR tete = monome();
    tete->coef *= signe;

    POINTEUR queue = tete;
    while (car == '+' || car == '-') {
        int s = (car == '-') ? -1 : 1;
        lire();
        POINTEUR m = monome();
        m->coef *= s;
        queue->suivant = m;
        queue = m;
    }
    return tete;
}

/* ------------------------------------------------------------------
   Fonction de debug : affiche la liste brute pour verification
   visuelle. Sera remplacee par un affichage propre a la Q3.
   ------------------------------------------------------------------ */
static void afficheDebug(POINTEUR p) {
    if (p == NULL) {
        printf("(liste vide)\n");
        return;
    }
    while (p != NULL) {
        printf("(%g, %d)", p->coef, p->expo);
        if (p->suivant != NULL)
            printf(" -> ");
        p = p->suivant;
    }
    printf(" -> NULL\n");
}

int main(void) {
    lire();
    POINTEUR p = polynome();
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le polynome");
    printf("polynome reconnu\n");
    printf("liste construite : ");
    afficheDebug(p);
    return 0;
}
