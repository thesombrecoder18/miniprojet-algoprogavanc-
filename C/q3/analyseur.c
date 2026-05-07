/*
 * Question 3 : Affichage d'un polynome.
 *
 * Ce fichier reprend la Q2 et y AJOUTE une fonction 'affiche' qui
 * imprime le polynome sous une forme lisible par un humain, du
 * genre "-4.5*X^5 + 2*X^4 + X^3 - X + 123".
 *
 * Regles d'affichage appliquees pour produire un texte propre :
 *   - liste vide                  -> "0"
 *   - premier monome negatif      -> commence par "-" (sans espace)
 *   - premier monome positif      -> aucun signe
 *   - monomes suivants            -> " + " ou " - " selon le signe
 *   - coefficient 1 avec expo > 0 -> on n'ecrit pas le 1
 *     (ex : "X^3" au lieu de "1*X^3")
 *   - exposant 0                  -> seul le coefficient apparait
 *   - exposant 1                  -> "X" au lieu de "X^1"
 *
 * Le reste (lecture, construction de la liste) est inchange par
 * rapport a la Q2.
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
   Affichage d'un polynome (Q3).

   Parcourt la liste chainee et imprime chaque monome en respectant
   les regles d'affichage decrites en haut du fichier.
   ------------------------------------------------------------------ */
static void affiche(POINTEUR p) {
    /* Cas particulier : polynome nul represente par la liste vide. */
    if (p == NULL) {
        printf("0");
        return;
    }

    int premier = 1; /* indique si on est sur le tout premier monome */
    while (p != NULL) {
        double c = p->coef;
        int    e = p->expo;

        /* Etape 1 : afficher le separateur (signe ou rien). */
        if (premier) {
            /* Premier monome : on prefixe par '-' uniquement si
               le coefficient est negatif. Pas de '+' explicite. */
            if (c < 0)
                printf("-");
            premier = 0;
        } else {
            /* Monomes suivants : " + " ou " - " selon le signe. */
            if (c < 0)
                printf(" - ");
            else
                printf(" + ");
        }

        /* Etape 2 : afficher le coefficient (en valeur absolue, le
           signe a deja ete imprime ci-dessus). */
        double abs_c = (c < 0) ? -c : c;

        if (e == 0) {
            /* Pas de X : on ecrit toujours le coefficient. */
            printf("%g", abs_c);
        } else {
            /* On a une partie en X. On omet le coefficient s'il
               vaut 1 (ex : "X^3" au lieu de "1*X^3"). */
            if (abs_c != 1.0)
                printf("%g*", abs_c);
            /* On omet l'exposant s'il vaut 1 (ex : "X" au lieu
               de "X^1"). */
            if (e == 1)
                printf("X");
            else
                printf("X^%d", e);
        }

        p = p->suivant;
    }
}

int main(void) {
    lire();
    POINTEUR p = polynome();
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le polynome");
    printf("polynome reconnu : ");
    affiche(p);
    printf("\n");
    return 0;
}
